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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#if USE_BACKEND
void Processor::debugProcessor(const String &t)
{

	if(consoleEnabled) debugToConsole(this, t);

};

#endif

void Processor::restoreFromValueTree(const ValueTree &previouslyExportedProcessorState)
{

	const ValueTree &v = previouslyExportedProcessorState;

	jassert(Identifier(v.getProperty("Type", String())) == getType());

	jassert(v.getProperty("ID", String()) == getId());
	setBypassed(v.getProperty("Bypassed", false));

	ScopedPointer<XmlElement> editorValueSet = v.getChildWithName("EditorStates").createXml();
	
	if(editorValueSet != nullptr)
	{
		if (!editorValueSet->hasAttribute("Visible") && (dynamic_cast<Chain*>(this) == nullptr || dynamic_cast<ModulatorSynth*>(this) != nullptr)) editorValueSet->setAttribute("Visible", true); // Compatibility for old patches

		editorStateValueSet.setFromXmlAttributes(*editorValueSet);
	}

	ValueTree childProcessors = v.getChildWithName("ChildProcessors");

	jassert(childProcessors.isValid());

	if(Chain *c = dynamic_cast<Chain*>(this))
	{
		if( !c->restoreChain(childProcessors)) return;
	}

	for(int i = 0; i < getNumChildProcessors(); i++)
	{
		Processor *p = getChildProcessor(i);

		for (int j = 0; j < childProcessors.getNumChildren(); j++)
		{
			if (childProcessors.getChild(j).getProperty("ID") == p->getId())
			{
				p->restoreFromValueTree(childProcessors.getChild(j));
				break;
			}
		}	
	}
};

void Processor::setConstrainerForAllInternalChains(BaseConstrainer *constrainer)
{
	FactoryType::Constrainer* c = static_cast<FactoryType::Constrainer*>(constrainer);

	for (int i = 0; i < getNumInternalChains(); i++)
	{
		ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));

		if (mc != nullptr)
		{
			mc->getFactoryType()->setConstrainer(c, false);

			for (int j = 0; j < mc->getNumChildProcessors(); j++)
			{
				mc->getChildProcessor(j)->setConstrainerForAllInternalChains(constrainer);
			}
		}
	}
}

const Identifier Processor::getIdentifierForParameterIndex(int parameterIndex) const
{
	if (auto pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(this))
	{
		if (auto content = pwsc->getScriptingContent())
		{
			return content->getComponent(parameterIndex)->getName();
		}
		else
			return Identifier();
	}
	else
	{
		if (parameterIndex > parameterNames.size()) return Identifier();

		return parameterNames[parameterIndex];
	}
}

int Processor::getNumParameters() const
{
	if (auto pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(this))
	{
		if (auto content = pwsc->getScriptingContent())
		{
			return content->getNumComponents();
		}
		else
			return 0;
	}
	else
		return parameterNames.size();
}

void Processor::setIsOnAir(bool isBeingProcessedInAudioThread)
{
	onAir = isBeingProcessedInAudioThread;

	for (int i = 0; i < getNumChildProcessors(); i++)
	{
		getChildProcessor(i)->setIsOnAir(isBeingProcessedInAudioThread);
	}
}


bool Chain::restoreChain(const ValueTree &v)
{
	Processor *thisAsProcessor = dynamic_cast<Processor*>(this);

	jassert(thisAsProcessor != nullptr);


	jassert(v.getType().toString() == "ChildProcessors");

	for (int i = 0; i < getHandler()->getNumProcessors(); i++)
	{
		getHandler()->getProcessor(i)->sendDeleteMessage();
	}

	getHandler()->clear();

	for(int i = 0; i < v.getNumChildren(); i++)
	{
		const bool isFixedInternalChain = i < thisAsProcessor->getNumChildProcessors();

		const bool isNoProcessorChild = v.getChild(i).getType() != Identifier("Processor");

		if (isNoProcessorChild || isFixedInternalChain)
		{
			continue; // They will be restored in Processor::restoreFromValueTree
		}
		else
		{

			Processor *p = MainController::createProcessor(getFactoryType(), v.getChild(i).getProperty("Type", String()).toString(), v.getChild(i).getProperty("ID"));
			
			if(p == nullptr)
			{
				String errorMessage;
				
				errorMessage << "The Processor (" << v.getChild(i).getType().toString() << ") " << v.getChild(i).getProperty("ID").toString() << "could not be generated. Skipping!";

				return false;
			}
			else
			{
				getHandler()->add(p, nullptr);
			}
		}
	}

	jassert(v.getNumChildren() == thisAsProcessor->getNumChildProcessors());

	return (v.getNumChildren() == thisAsProcessor->getNumChildProcessors());
};

bool FactoryType::countProcessorsWithSameId(int &index, const Processor *p, Processor *processorToLookFor, const String &nameToLookFor)
{   
	

	if (p->getId().startsWith(nameToLookFor))
	{
		index++;
	}

	if (p == processorToLookFor)
	{
		// Do not look
		return false;
	}
	
	const int numChildren = p->getNumChildProcessors();

	for (int i = 0; i < numChildren; i++)
	{
		bool lookFurther = countProcessorsWithSameId(index, p->getChildProcessor(i), processorToLookFor, nameToLookFor);

		if (!lookFurther) return false;
	}
	
	return true;
}


String FactoryType::getUniqueName(Processor *id, String name/*=String()*/)
{
	ModulatorSynthChain *chain = id->getMainController()->getMainSynthChain();

	if (id == chain) return chain->getId();

	int amount = 0;

	if(name.isEmpty()) name = id->getId();

	countProcessorsWithSameId(amount, chain, id, name);

	if(amount > 0) name = name + String(amount+1);
	else name = id->getId();

	return name;
}


Processor *ProcessorHelpers::getFirstProcessorWithName(const Processor *root, const String &name)
{
	Processor::Iterator<Processor> iter(const_cast<Processor*>(root), false);

	Processor *p;

	while((p = iter.getNextProcessor()) != nullptr)
	{
		if(p->getId() == name) return p;
	}

	return nullptr;
}

const Processor *ProcessorHelpers::findParentProcessor(const Processor *childProcessor, bool getParentSynth)
{
	return const_cast<const Processor*>(findParentProcessor(const_cast<Processor*>(childProcessor), getParentSynth));
}



Processor * ProcessorHelpers::findParentProcessor(Processor *childProcessor, bool getParentSynth)
{
	Processor *root = const_cast<Processor*>(childProcessor)->getMainController()->getMainSynthChain();
	Processor::Iterator<Processor> iter(root, false);

	Processor *p;
	Processor *lastSynth = nullptr;

	if (getParentSynth)
	{
		while ((p = iter.getNextProcessor()) != nullptr)
		{
			if (is<ModulatorSynth>(childProcessor))
			{
				if (is<Chain>(p))
				{
					Chain::Handler *handler = dynamic_cast<Chain*>(p)->getHandler();
					int numChildSynths = handler->getNumProcessors();

					for (int i = 0; i < numChildSynths; i++)
					{
						if (handler->getProcessor(i) == childProcessor) return p;
					}
				}
			}
			else
			{
				if (is<ModulatorSynth>(p)) lastSynth = p;

				if (p == childProcessor) return lastSynth;
			}
		}
	}
	else
	{
		while ((p = iter.getNextProcessor()) != nullptr)
		{
			for (int i = 0; i < p->getNumChildProcessors(); i++)
			{
				if (p->getChildProcessor(i) == childProcessor) return p;
			}
		}
	}

	return nullptr;
}

template <class ProcessorType>
int ProcessorHelpers::getAmountOf(const Processor *rootProcessor, const Processor *upTochildProcessor /*= nullptr*/)
{
	Processor::Iterator<ProcessorType> iter(rootProcessor);

	int count = 0;

	while (ProcessorType *p = iter.getNextProcessor())
	{
		
		if (upTochildProcessor != nullptr && p == upTochildProcessor)
		{
			break;
		}

		count++;
	}

	return count;
}



bool ProcessorHelpers::isHiddableProcessor(const Processor *p)
{
	return dynamic_cast<const Chain*>(p) == nullptr ||
		   dynamic_cast<const ModulatorSynthChain*>(p) != nullptr ||
		   dynamic_cast<const ModulatorSynthGroup*>(p) != nullptr;
}

String ProcessorHelpers::getScriptVariableDeclaration(const Processor *p, bool copyToClipboard/*=true*/)
{
	String typeName;

	if (is<ModulatorSynth>(p)) typeName = "ChildSynth";
	else if (is<Modulator>(p)) typeName = "Modulator";
	else if (is<MidiProcessor>(p)) typeName = "MidiProcessor";
	else if (is<EffectProcessor>(p)) typeName = "Effect";
	else return String();

	String code;

	String name = p->getId();
	String id = name.removeCharacters(" \n\t\"\'!$%&/()");
	
	code << "const var " << id << " = Synth.get" << typeName << "(\"" << name << "\");";

	if (copyToClipboard)
	{
		debugToConsole(const_cast<Processor*>(p), "'" + code + "' was copied to Clipboard");
		SystemClipboard::copyTextToClipboard(code);
	}

	return code;
}

String ProcessorHelpers::getBase64String(const Processor* p, bool copyToClipboard/*=true*/, bool exportContentOnly/*=false*/)
{
	if (exportContentOnly)
	{
		if (auto pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(p))
		{
			auto vt = pwsc->getScriptingContent()->exportAsValueTree();
			return ValueTreeHelpers::getBase64StringFromValueTree(vt);
		}
		else
			return String();
	}
	else
	{
		ValueTree v;

		v = p->exportAsValueTree();

		const String c = ValueTreeHelpers::getBase64StringFromValueTree(v);

		if (copyToClipboard)
			SystemClipboard::copyTextToClipboard("\"" + c + "\"");

		return c;
	}

	
}

void ProcessorHelpers::restoreFromBase64String(Processor* p, const String& base64String, bool restoreScriptContentOnly/*=false*/)
{
	if (restoreScriptContentOnly)
	{
		if (auto pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(p))
		{
			auto vt = ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(base64String);

			if (auto content = pwsc->getScriptingContent())
				content->restoreAllControlsFromPreset(vt);
		}
	}
	else
	{
		ValueTree v = ValueTreeHelpers::getValueTreeFromBase64String(base64String);

		auto newId = v.getProperty("ID", String()).toString();

		if (newId.isNotEmpty())
			p->setId(newId, dontSendNotification);

		p->restoreFromValueTree(v);

		if (auto firstChild = p->getChildProcessor(0))
			firstChild->sendRebuildMessage(true);
	}
	
}

void ProcessorHelpers::deleteProcessor(Processor* p)
{
	PresetHandler::setChanged(p);

	p->sendDeleteMessage();

	auto c = dynamic_cast<Chain*>(findParentProcessor(p, false));

	if (c != nullptr)
	{
		c->getHandler()->remove(p);
	}
}

void ProcessorHelpers::increaseBufferIfNeeded(AudioSampleBuffer& b, int numSamplesNeeded)
{
	// The channel amount must be set correctly in the constructor
	jassert(b.getNumChannels() > 0);

	if (b.getNumSamples() < numSamplesNeeded)
	{
		b.setSize(b.getNumChannels(), numSamplesNeeded, true, true, true);
		b.clear();
	}
}

void ProcessorHelpers::increaseBufferIfNeeded(hlac::HiseSampleBuffer& b, int numSamplesNeeded)
{
	// The channel amount must be set correctly in the constructor
	jassert(b.getNumChannels() > 0);

	if (b.getNumSamples() < numSamplesNeeded)
	{
		b.setSize(b.getNumChannels(), numSamplesNeeded);
		b.clear();
	}
}

void AudioSampleProcessor::replaceReferencesWithGlobalFolder()
{
	if (!isReference(loadedFileName))
	{
		loadedFileName = getGlobalReferenceForFile(loadedFileName);
	}
}

void AudioSampleProcessor::setLoadedFile(const String &fileName, bool loadThisFile/*=false*/, bool forceReload/*=false*/)
{
	ignoreUnused(forceReload);

	loadedFileName = fileName;

	if (fileName.isEmpty())
	{
		length = 0;
		sampleRateOfLoadedFile = -1.0;
		sampleBuffer = nullptr;

		setRange(Range<int>(0, 0));

		// A AudioSampleProcessor must also be derived from Processor!
		jassert(dynamic_cast<Processor*>(this) != nullptr);

		dynamic_cast<Processor*>(this)->sendChangeMessage();

		newFileLoaded();
	}

	if(loadThisFile && fileName.isNotEmpty())
	{
		ScopedLock sl(getFileLock());

		mc->getSampleManager().getAudioSampleBufferPool()->releasePoolData(sampleBuffer);

#if USE_FRONTEND

		sampleBuffer = mc->getSampleManager().getAudioSampleBufferPool()->loadFileIntoPool(fileName, false);

		Identifier fileId = mc->getSampleManager().getAudioSampleBufferPool()->getIdForFileName(fileName);

#else

		File actualFile = getFile(loadedFileName, PresetPlayerHandler::AudioFiles);
		Identifier fileId = mc->getSampleManager().getAudioSampleBufferPool()->getIdForFileName(actualFile.getFullPathName());
		sampleBuffer = mc->getSampleManager().getAudioSampleBufferPool()->loadFileIntoPool(actualFile.getFullPathName(), forceReload);

#endif

		sampleRateOfLoadedFile = mc->getSampleManager().getAudioSampleBufferPool()->getSampleRateForFile(fileId);

		if(sampleBuffer != nullptr)
		{
			setRange(Range<int>(0, sampleBuffer->getNumSamples()));
		}

		// A AudioSampleProcessor must also be derived from Processor!
		jassert(dynamic_cast<Processor*>(this) != nullptr);
		
		dynamic_cast<Processor*>(this)->sendChangeMessage();

		newFileLoaded();
	}
};

void AudioSampleProcessor::setRange(Range<int> newSampleRange)
{
	if(!newSampleRange.isEmpty() && sampleBuffer != nullptr)
	{
		jassert(sampleBuffer != nullptr);

		ScopedLock sl(getFileLock());

		sampleRange = newSampleRange;
		sampleRange.setEnd(jmin<int>(sampleBuffer->getNumSamples(), sampleRange.getEnd()));
		length = sampleRange.getLength();

		rangeUpdated();
		
		dynamic_cast<Processor*>(this)->sendChangeMessage();
	}
};


String ProcessorHelpers::ValueTreeHelpers::getBase64StringFromValueTree(const ValueTree& v)
{
	MemoryOutputStream internalMos;
	GZIPCompressorOutputStream gzos(&internalMos, 9, false);
	MemoryOutputStream mos;

	v.writeToStream(mos);

	gzos.write(mos.getData(), mos.getDataSize());
	gzos.flush();

	return internalMos.getMemoryBlock().toBase64Encoding();
}

ValueTree ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(const String& base64State)
{
	MemoryBlock mb;

	mb.fromBase64Encoding(base64State);

	return ValueTree::readFromGZIPData(mb.getData(), mb.getSize());
}