/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace hise {
using namespace juce;

struct MidiPlayerHelpers
{
	static double samplesToSeconds(double samples, double sr)
	{
		return samples / sr;
	}

	static double samplesToTicks(double samples, double bpm, double sr)
	{
		auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sr, TempoSyncer::Quarter);
		return (double)HiseMidiSequence::TicksPerQuarter * samples / samplesPerQuarter;
	}

	static double secondsToTicks(double seconds, double bpm, double sr)
	{
		auto samples = secondsToSamples(seconds, sr);
		return samplesToTicks(samples, bpm, sr);
	}

	static double secondsToSamples(double seconds, double sr)
	{
		return seconds * sr;
	}

	static double ticksToSamples(double ticks, double bpm, double sr)
	{
		auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sr, TempoSyncer::Quarter);

		return samplesPerQuarter * ticks / HiseMidiSequence::TicksPerQuarter;
	}
};



HiseMidiSequence::SimpleReadWriteLock::ScopedReadLock::ScopedReadLock(SimpleReadWriteLock &lock_) :
	lock(lock_)
{
	for (int i = 20; --i >= 0;)
		if (!lock.isBeingWritten)
			break;

	while (lock.isBeingWritten)
		Thread::yield();

	lock.numReadLocks++;
}

HiseMidiSequence::SimpleReadWriteLock::ScopedReadLock::~ScopedReadLock()
{
	lock.numReadLocks--;
}

HiseMidiSequence::SimpleReadWriteLock::ScopedWriteLock::ScopedWriteLock(SimpleReadWriteLock &lock_) :
	lock(lock_)
{
	if (lock.isBeingWritten)
	{
		// jassertfalse;
		return;
	}

	for (int i = 100; --i >= 0;)
		if (lock.numReadLocks == 0)
			break;

	while (lock.numReadLocks > 0)
		Thread::yield();

	lock.isBeingWritten = true;
}

HiseMidiSequence::SimpleReadWriteLock::ScopedWriteLock::~ScopedWriteLock()
{
	lock.isBeingWritten = false;
}


HiseMidiSequence::HiseMidiSequence()
{

}



juce::ValueTree HiseMidiSequence::exportAsValueTree() const
{
	ValueTree v("MidiFile");
	v.setProperty("ID", id.toString(), nullptr);

	MemoryOutputStream mos;

	MidiFile currentFile;

	for (auto t : sequences)
		currentFile.addTrack(*t);

	currentFile.writeTo(mos);
	auto data = mos.getMemoryBlock();
	zstd::ZDefaultCompressor compressor;
	compressor.compressInplace(data);
	v.setProperty("Data", data.toBase64Encoding(), nullptr);

	return v;
}

void HiseMidiSequence::restoreFromValueTree(const ValueTree &v)
{
	id = v.getProperty("ID").toString();

	// This property isn't used in this class, but if you want to
	// have any kind of connection to a pooled MidiFile, you will
	// need to add this externally (see MidiPlayer::exportAsValueTree())
	jassert(v.getProperty("FileName"));

	String encodedState = v.getProperty("Data");

	MemoryBlock mb;

	if (mb.fromBase64Encoding(encodedState))
	{
		zstd::ZDefaultCompressor compressor;
		compressor.expandInplace(mb);
		MemoryInputStream mis(mb, false);
		MidiFile mf;
		mf.readFrom(mis);
		loadFrom(mf);
	}
}


juce::MidiMessage* HiseMidiSequence::getNextEvent(Range<double> rangeToLookForTicks)
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	auto nextIndex = lastPlayedIndex + 1;

	if (auto seq = getReadPointer(currentTrackIndex))
	{
		if (nextIndex >= seq->getNumEvents())
		{
			lastPlayedIndex = -1;
			nextIndex = 0;
		}

		if (auto nextEvent = seq->getEventPointer(nextIndex))
		{
			auto timestamp = nextEvent->message.getTimeStamp();

			auto maxLength = getLength();


			if (rangeToLookForTicks.contains(timestamp))
			{
				lastPlayedIndex = nextIndex;
				return &nextEvent->message;
			}
			else if (rangeToLookForTicks.contains(maxLength))
			{
				auto rangeAtBeginning = rangeToLookForTicks.getEnd() - maxLength;

				if (timestamp < rangeAtBeginning)
				{
					lastPlayedIndex = nextIndex;
					return &nextEvent->message;
				}
			}
		}
	}

	return nullptr;
}

juce::MidiMessage* HiseMidiSequence::getMatchingNoteOffForCurrentEvent()
{
	if (auto noteOff = getReadPointer(currentTrackIndex)->getEventPointer(lastPlayedIndex)->noteOffObject)
		return &noteOff->message;

	return nullptr;
}

double HiseMidiSequence::getLength() const
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	if (auto currentSequence = sequences.getFirst())
		return currentSequence->getEndTime();

	return 0.0;
}

double HiseMidiSequence::getLengthInQuarters()
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	if (auto currentSequence = sequences.getFirst())
		return currentSequence->getEndTime() / (double)TicksPerQuarter;

	return 0.0;
}


void HiseMidiSequence::loadFrom(const MidiFile& file)
{
	OwnedArray<MidiMessageSequence> newSequences;

	MidiFile normalisedFile;

	for (int i = 0; i < file.getNumTracks(); i++)
	{
		ScopedPointer<MidiMessageSequence> newSequence = new MidiMessageSequence(*file.getTrack(i));
		newSequence->deleteSysExMessages();

		DBG("Track " + String(i + 1));

		for (int j = 0; j < newSequence->getNumEvents(); j++)
		{
			if (newSequence->getEventPointer(j)->message.isMetaEvent())
				newSequence->deleteEvent(j--, false);
		}

		if (newSequence->getNumEvents() > 0)
			normalisedFile.addTrack(*newSequence);
	}

	normalisedFile.setTicksPerQuarterNote(TicksPerQuarter);

	for (int i = 0; i < normalisedFile.getNumTracks(); i++)
	{
		ScopedPointer<MidiMessageSequence> newSequence = new MidiMessageSequence(*normalisedFile.getTrack(i));
		newSequences.add(newSequence.release());
	}

	{
		SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
		newSequences.swapWith(sequences);
	}
}

juce::File HiseMidiSequence::writeToTempFile()
{
	MidiFile f;

	for (int i = 0; i < sequences.size(); i++)
	{
		f.addTrack(*sequences[i]);
	}

	auto tmp = File::getSpecialLocation(File::SpecialLocationType::tempDirectory).getNonexistentChildFile(id.toString(), ".mid");
	tmp.create();

	FileOutputStream fos(tmp);
	f.writeTo(fos);
	return tmp;
}

void HiseMidiSequence::setId(const Identifier& newId)
{
	id = newId;
}

const juce::MidiMessageSequence* HiseMidiSequence::getReadPointer(int trackIndex) const
{
	if (trackIndex == -1)
		return sequences[currentTrackIndex];

	return sequences[trackIndex];
}

juce::MidiMessageSequence* HiseMidiSequence::getWritePointer(int trackIndex)
{
	if (trackIndex == -1)
		return sequences[currentTrackIndex];

	return sequences[trackIndex];
}

int HiseMidiSequence::getNumEvents() const
{
	return sequences[currentTrackIndex]->getNumEvents();
}

void HiseMidiSequence::setCurrentTrackIndex(int index)
{
	if (index != currentTrackIndex)
	{
		double lastTimestamp = 0.0;

		SimpleReadWriteLock::ScopedReadLock sl(swapLock);

		if (lastPlayedIndex != -1)
			lastTimestamp = getReadPointer(currentTrackIndex)->getEventPointer(lastPlayedIndex)->message.getTimeStamp();

		currentTrackIndex = jlimit<int>(0, sequences.size()-1, index);

		if (lastPlayedIndex != -1)
			lastPlayedIndex = getReadPointer(currentTrackIndex)->getNextIndexAtTime(lastTimestamp);
	}
}

void HiseMidiSequence::resetPlayback()
{
	lastPlayedIndex = -1;
}

void HiseMidiSequence::setPlaybackPosition(double normalisedPosition)
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	if (auto s = getReadPointer(currentTrackIndex))
	{
		auto currentTimestamp = getLength() * normalisedPosition;

		lastPlayedIndex = s->getNextIndexAtTime(currentTimestamp) - 1;
	}
}

juce::RectangleList<float> HiseMidiSequence::getRectangleList(Rectangle<float> targetBounds) const
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	RectangleList<float> list;

	if (auto s = getReadPointer(currentTrackIndex))
	{
		for (auto e : *s)
		{
			if (e->message.isNoteOn() && e->noteOffObject != nullptr)
			{
				auto x = (float)(e->message.getTimeStamp() / getLength());
				auto w = (float)(e->noteOffObject->message.getTimeStamp() / getLength()) - x;
				auto y = (float)(127 - e->message.getNoteNumber());
				auto h = 1.0f;

				list.add({ x, y, w, h });
			}
		}
	}

	if (!targetBounds.isEmpty())
	{
		auto bounds = list.getBounds();
		list.offsetAll(0.0f, -bounds.getY());
		auto scaler = AffineTransform::scale(targetBounds.getWidth() / bounds.getRight(), targetBounds.getHeight() / bounds.getHeight());
		list.transformAll(scaler);
	}

	return list;
}


Array<HiseEvent> HiseMidiSequence::getEventList(double sampleRate, double bpm)
{
	Array<HiseEvent> newBuffer;
	newBuffer.ensureStorageAllocated(getNumEvents());

	Range<double> range = { 0.0, getLength() };

	auto samplePerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);

	uint16 eventIds[127];
	uint16 currentEventId = 1;
	memset(eventIds, 0, sizeof(uint16) * 127);

	auto mSeq = getReadPointer();

	for (const auto& ev : *mSeq)
	{
		auto m = ev->message;

		auto timeStamp = (int)(samplePerQuarter * m.getTimeStamp() / (double)HiseMidiSequence::TicksPerQuarter);
		HiseEvent newEvent(m);
		newEvent.setTimeStamp(timeStamp);

		if (newEvent.isNoteOn())
		{
			newEvent.setEventId(currentEventId);
			eventIds[newEvent.getNoteNumber()] = currentEventId++;
		}
		if (newEvent.isNoteOff())
			newEvent.setEventId(eventIds[newEvent.getNoteNumber()]);

		newBuffer.add(newEvent);
	}

	return newBuffer;
}



void HiseMidiSequence::swapCurrentSequence(MidiMessageSequence* sequenceToSwap)
{
	ScopedPointer<MidiMessageSequence> oldSequence = sequences[currentTrackIndex];

	{
		SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
		sequences.set(currentTrackIndex, sequenceToSwap, false);
	}
	
	oldSequence = nullptr;
}


MidiFilePlayer::EditAction::EditAction(WeakReference<MidiFilePlayer> currentPlayer_, const Array<HiseEvent>& newContent, double sampleRate_, double bpm_) :
	UndoableAction(),
	currentPlayer(currentPlayer_),
	newEvents(newContent),
	sampleRate(sampleRate_),
	bpm(bpm_)
{
	oldEvents = currentPlayer->getCurrentSequence()->getEventList(sampleRate, bpm);

	if (currentPlayer == nullptr)
		return;

	if (auto seq = currentPlayer->getCurrentSequence())
		sequenceId = seq->getId();
}

bool MidiFilePlayer::EditAction::perform()
{
	if (currentPlayer != nullptr && currentPlayer->getSequenceId() == sequenceId)
	{
		writeArrayToSequence(newEvents);
		return true;
	}
		
	return false;
}

bool MidiFilePlayer::EditAction::undo()
{
	if (currentPlayer != nullptr && currentPlayer->getSequenceId() == sequenceId)
	{
		writeArrayToSequence(oldEvents);
		return true;
	}

	return false;
}

void MidiFilePlayer::EditAction::writeArrayToSequence(Array<HiseEvent>& arrayToWrite)
{
	ScopedPointer<MidiMessageSequence> newSeq = new MidiMessageSequence();

	auto samplePerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);

	for (const auto& e : arrayToWrite)
	{
		auto timeStamp = ((double)e.getTimeStamp() / samplePerQuarter) * (double)HiseMidiSequence::TicksPerQuarter;
		auto m = e.toMidiMesage();
		m.setTimeStamp(timeStamp);
		newSeq->addEvent(m);
	}

	newSeq->sort();
	newSeq->updateMatchedPairs();

	currentPlayer->swapCurrentSequence(newSeq.release());
}



MidiFilePlayer::MidiFilePlayer(MainController *mc, const String &id, ModulatorSynth* ) :
	MidiProcessor(mc, id),
	undoManager(new UndoManager())
{
	addAttributeID(Stop);
	addAttributeID(Play);
	addAttributeID(Record);
	addAttributeID(CurrentPosition);
	addAttributeID(CurrentSequence);
	addAttributeID(CurrentTrack);
	addAttributeID(ClearSequences);

	mc->addTempoListener(this);

}

MidiFilePlayer::~MidiFilePlayer()
{
	getMainController()->removeTempoListener(this);
}

void MidiFilePlayer::tempoChanged(double newTempo)
{
	ticksPerSample = MidiPlayerHelpers::samplesToTicks(1, newTempo, getSampleRate());
}

juce::ValueTree MidiFilePlayer::exportAsValueTree() const
{
	ValueTree v = MidiProcessor::exportAsValueTree();

	saveID(CurrentSequence);
	saveID(CurrentTrack);
	saveID(LoopEnabled);

	ValueTree seq("MidiFiles");

	for (int i = 0; i < currentSequences.size(); i++)
	{
		auto s = currentSequences[i]->exportAsValueTree();
		s.setProperty("FileName", currentlyLoadedFiles[i].getReferenceString(), nullptr);

		seq.addChild(s, -1, nullptr);
	}
		
	v.addChild(seq, -1, nullptr);

	return v;
}

void MidiFilePlayer::restoreFromValueTree(const ValueTree &v)
{
	MidiProcessor::restoreFromValueTree(v);

	ValueTree seq = v.getChildWithName("MidiFiles");

	clearSequences(dontSendNotification);

	if (seq.isValid())
	{
		for (const auto& s : seq)
		{
			HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();
			newSequence->restoreFromValueTree(s);

			PoolReference ref(getMainController(), s.getProperty("FileName", ""), FileHandlerBase::MidiFiles);

			currentlyLoadedFiles.add(ref);

			addSequence(newSequence, false);
		}
	}

	loadID(CurrentSequence);
	loadID(CurrentTrack);
	loadID(LoopEnabled);
}

void MidiFilePlayer::addSequence(HiseMidiSequence::Ptr newSequence, bool select)
{
	currentSequences.add(newSequence);

	if (select)
	{
		currentSequenceIndex = currentSequences.size() - 1;
		sendChangeMessage();
	}

	for (auto l : sequenceListeners)
	{
		if (l != nullptr)
			l->sequenceLoaded(newSequence);
	}
}

void MidiFilePlayer::clearSequences(NotificationType notifyListeners)
{
	currentSequences.clear();
	currentlyLoadedFiles.clear();

	currentSequenceIndex = -1;

	if (notifyListeners != dontSendNotification)
	{
		for (auto l : sequenceListeners)
		{
			if (l != nullptr)
				l->sequencesCleared();
		}
	}
}

hise::ProcessorEditorBody * MidiFilePlayer::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new MidiFilePlayerEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

float MidiFilePlayer::getAttribute(int index) const
{
	auto s = (SpecialParameters)index;

	switch (s)
	{
	case CurrentPosition:		return (float)getPlaybackPosition();
	case CurrentSequence:		return (float)(currentSequenceIndex + 1);
	case CurrentTrack:			return (float)(currentTrackIndex + 1);
	case LoopEnabled:			return loopEnabled ? 1.0f : 0.0f;
	default:
		break;
	}

	return 0.0f;
}

void MidiFilePlayer::setInternalAttribute(int index, float newAmount)
{
	auto s = (SpecialParameters)index;

	switch (s)
	{
	case CurrentPosition:		
	{
		currentPosition = jlimit<double>(0.0, 1.0, (double)newAmount); 

		updatePositionInCurrentSequence();
		break;
	}
	case CurrentSequence:		
	{
		double lastLength = 0.0f;

		if (auto seq = getCurrentSequence())
			lastLength = seq->getLengthInQuarters();

		currentSequenceIndex = jlimit<int>(-1, currentSequences.size(), (int)(newAmount - 1)); 

		if (auto seq = getCurrentSequence())
		{
			double newLength = seq->getLengthInQuarters();

			if (newLength > 0.0 && currentPosition >= 0.0)
			{
				double ratio = lastLength / newLength;
				currentPosition = currentPosition * ratio;
				
				updatePositionInCurrentSequence();
			}
		}

		break;
	}
	case CurrentTrack:			
	{
		currentTrackIndex = jmax<int>(0, (int)(newAmount - 1)); 
		getCurrentSequence()->setCurrentTrackIndex(currentTrackIndex);
		break;
	}
	case LoopEnabled: loopEnabled = newAmount > 0.5f; break;
	default:
		break;
	}
}


void MidiFilePlayer::loadMidiFile(PoolReference reference)
{
	auto newContent = getMainController()->getCurrentMidiFilePool(true)->loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);

	currentlyLoadedFiles.add(reference);

	HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();
	newSequence->loadFrom(newContent->data.getFile());
	newSequence->setId(reference.getFile().getFileNameWithoutExtension());
	addSequence(newSequence);
	
}

void MidiFilePlayer::prepareToPlay(double sampleRate_, int samplesPerBlock_)
{
	MidiProcessor::prepareToPlay(sampleRate_, samplesPerBlock_);
	tempoChanged(getMainController()->getBpm());
}

void MidiFilePlayer::preprocessBuffer(HiseEventBuffer& buffer, int numSamples)
{
	if (currentSequenceIndex >= 0 && currentPosition != -1.0)
	{
		if (!loopEnabled && currentPosition > 1.0)
		{
			stop();
			return;
		}

		auto seq = getCurrentSequence();
		seq->setCurrentTrackIndex(currentTrackIndex);

		auto tickThisTime = (numSamples - timeStampForNextCommand) * ticksPerSample;
		auto lengthInTicks = seq->getLength();

		auto positionInTicks = getPlaybackPosition() * lengthInTicks;

		auto delta = tickThisTime / lengthInTicks;

		Range<double> currentRange;
		
		if(loopEnabled)
			currentRange = { positionInTicks, positionInTicks + tickThisTime };
		else
			currentRange = { positionInTicks, jmin<double>(lengthInTicks, positionInTicks + tickThisTime) };

		while (auto e = seq->getNextEvent(currentRange))
		{
			auto timeStampInThisBuffer = e->getTimeStamp() - positionInTicks;

			if (timeStampInThisBuffer < 0.0)
				timeStampInThisBuffer += lengthInTicks;

			auto timeStamp = (int)MidiPlayerHelpers::ticksToSamples(timeStampInThisBuffer, getMainController()->getBpm(), getSampleRate());
			timeStamp += timeStampForNextCommand;

			jassert(isPositiveAndBelow(timeStamp, numSamples));

			HiseEvent newEvent(*e);

			newEvent.setTimeStamp(timeStamp);
			newEvent.setArtificial();

			if (newEvent.isNoteOn())
			{
				getMainController()->getEventHandler().pushArtificialNoteOn(newEvent);

				buffer.addEvent(newEvent);

				if (auto noteOff = seq->getMatchingNoteOffForCurrentEvent())
				{
					HiseEvent newNoteOff(*noteOff);
					newNoteOff.setArtificial();

					auto noteOffTimeStampInBuffer = noteOff->getTimeStamp() - positionInTicks;
					
					if (noteOffTimeStampInBuffer < 0.0)
						noteOffTimeStampInBuffer += lengthInTicks;

					timeStamp += timeStampForNextCommand;

					auto noteOffTimeStamp = (int)MidiPlayerHelpers::ticksToSamples(noteOffTimeStampInBuffer, getMainController()->getBpm(), getSampleRate());

					auto on_id = getMainController()->getEventHandler().getEventIdForNoteOff(newNoteOff);

					jassert(newEvent.getEventId() == on_id);

					newNoteOff.setEventId(on_id);
					newNoteOff.setTimeStamp(noteOffTimeStamp);

					if (noteOffTimeStamp < numSamples)
						buffer.addEvent(newNoteOff);
					else
						addHiseEventToBuffer(newNoteOff);
				}
			}
		}

		timeStampForNextCommand = 0;
		currentPosition += delta;
	}
}

void MidiFilePlayer::processHiseEvent(HiseEvent &m) noexcept
{
	currentTimestampInBuffer = m.getTimeStamp();
}

void MidiFilePlayer::addSequenceListener(SequenceListener* newListener)
{
	sequenceListeners.addIfNotAlreadyThere(newListener);
}

void MidiFilePlayer::removeSequenceListener(SequenceListener* listenerToRemove)
{
	sequenceListeners.removeAllInstancesOf(listenerToRemove);
}


void MidiFilePlayer::sendSequenceUpdateMessage(NotificationType notification)
{
	auto update = [this]()
	{
		for (auto l : sequenceListeners)
		{
			if (l != nullptr)
				l->sequenceLoaded(getCurrentSequence());
		}
	};

	if (notification == sendNotificationAsync)
		MessageManager::callAsync(update);
	else
		update();
}

void MidiFilePlayer::changeTransportState(PlayState newState)
{
	switch (newState)
	{						// Supposed to be immediately...
	case PlayState::Play:	play(0);	return;
	case PlayState::Stop:	stop(0);	return;
	case PlayState::Record: record(0);	return;
	}
}

hise::HiseMidiSequence* MidiFilePlayer::getCurrentSequence() const
{
	return currentSequences[currentSequenceIndex].get();
}

juce::Identifier MidiFilePlayer::getSequenceId(int index) const
{
	if (index == -1)
		index = currentSequenceIndex;

	if (auto s = currentSequences[index])
	{
		return s->getId();
	}

	return {};
}

double MidiFilePlayer::getPlaybackPosition() const
{
	return fmod(currentPosition, 1.0);
}

void MidiFilePlayer::swapCurrentSequence(MidiMessageSequence* newSequence)
{
	getCurrentSequence()->swapCurrentSequence(newSequence);

	updatePositionInCurrentSequence();
	sendSequenceUpdateMessage(sendNotificationAsync);
}


void MidiFilePlayer::setEnableUndoManager(bool shouldBeEnabled)
{
	bool isEnabled = undoManager != nullptr;

	if (isEnabled != shouldBeEnabled)
	{
		undoManager = nullptr;

		if (isEnabled)
			undoManager = new UndoManager();
	}
}

void MidiFilePlayer::flushEdit(const Array<HiseEvent>& newEvents)
{
	ScopedPointer<EditAction> newAction = new EditAction(this, newEvents, getSampleRate(), getMainController()->getBpm());

	if (undoManager != nullptr)
	{
		undoManager->beginNewTransaction();
		undoManager->perform(newAction.release());
	}
	else
		newAction->perform();
}

void MidiFilePlayer::resetCurrentSequence()
{
	if (auto seq = getCurrentSequence())
	{
		auto original = getMainController()->getCurrentMidiFilePool()->loadFromReference(currentlyLoadedFiles[currentSequenceIndex], PoolHelpers::LoadAndCacheWeak);

		ScopedPointer<HiseMidiSequence> tempSeq = new HiseMidiSequence();
		tempSeq->loadFrom(original->data.getFile());
		auto l = tempSeq->getEventList(getSampleRate(), getMainController()->getBpm());

		flushEdit(l);
	}
}

hise::PoolReference MidiFilePlayer::getPoolReference(int index /*= -1*/)
{
	if (index == -1)
		index = currentSequenceIndex;

	return currentlyLoadedFiles[index];
}

bool MidiFilePlayer::play(int timestamp)
{
	sendAllocationFreeChangeMessage();

	if (auto seq = getCurrentSequence())
	{
		playState = PlayState::Play;
		timeStampForNextCommand = timestamp;
		currentPosition = 0.0;
		seq->resetPlayback();
		return true;
	}
		
	return false;
}

bool MidiFilePlayer::stop(int timestamp)
{
	sendAllocationFreeChangeMessage();

	if (auto seq = getCurrentSequence())
	{
		seq->resetPlayback();
		playState = PlayState::Stop;
		timeStampForNextCommand = timestamp;
		currentPosition = -1.0;
		return true;
	}
		
	return false;
}

bool MidiFilePlayer::record(int timestamp)
{
	sendAllocationFreeChangeMessage();

	playState = PlayState::Record;
	timeStampForNextCommand = timestamp;

	// Not yet implemented
	jassertfalse;

	return false;
}


void MidiFilePlayer::updatePositionInCurrentSequence()
{
	if (auto seq = getCurrentSequence())
	{
		seq->setPlaybackPosition(getPlaybackPosition());
	}
}

MidiFilePlayerBaseType::~MidiFilePlayerBaseType()
{
	

	if (player != nullptr)
	{
		player->removeSequenceListener(this);
		player->removeChangeListener(this);
	}
}

MidiFilePlayerBaseType::MidiFilePlayerBaseType(MidiFilePlayer* player_) :
	player(player_),
	font(GLOBAL_BOLD_FONT())
{
	player->addSequenceListener(this);
	player->addChangeListener(this);
}

void MidiFilePlayerBaseType::changeListenerCallback(SafeChangeBroadcaster* )
{
	int thisSequence = (int)getPlayer()->getAttribute(MidiFilePlayer::CurrentSequence);

	if (thisSequence != lastSequenceIndex)
	{
		lastSequenceIndex = thisSequence;
		sequenceIndexChanged();
	}

	int trackIndex = (int)getPlayer()->getAttribute(MidiFilePlayer::CurrentTrack);

	if (trackIndex != lastTrackIndex)
	{
		lastTrackIndex = trackIndex;
		trackIndexChanged();
	}
}

}