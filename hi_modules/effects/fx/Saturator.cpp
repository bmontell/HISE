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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

SaturatorEffect::SaturatorEffect(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid),
	saturationChain(new ModulatorChain(mc, "Saturation Modulation", 1, Modulation::GainMode, this)),
	saturation(0.0f),
	wet(1.0f),
	dry(0.0f),
	preGain(1.0f),
    postGain(1.0f)
{
	parameterNames.add("Saturation");
	parameterNames.add("WetAmount");
	parameterNames.add("PreGain");
	parameterNames.add("PostGain");

	editorStateIdentifiers.add("SaturationChainShown");

	saturator.setSaturationAmount(0.0f);

	saturationChain->setFactoryType(new TimeVariantModulatorFactoryType(Modulation::GainMode, this));
}

void SaturatorEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Saturation:
		saturation = newValue;
		saturator.setSaturationAmount(newValue);
		break;
	case WetAmount:
		dry = 1.0f - newValue;
		wet = newValue;
		break;
	case PreGain:
		preGain =  Decibels::decibelsToGain(newValue);
		break;
	case PostGain:
		postGain = Decibels::decibelsToGain(newValue);
		break;
	default:
		break;
	}

}

float SaturatorEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Saturation:
		return saturation;
		break;
	case WetAmount:
		return wet;
		break;
	case PreGain:
		return Decibels::gainToDecibels(preGain);
	case PostGain:
		return Decibels::gainToDecibels(postGain);
	default:
		break;
	}

	jassertfalse;
	return 0.0f;
}

float SaturatorEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Saturation:
		return 0.0;
		break;
	case WetAmount:
		return 100.0;
		break;
	case PreGain:
		return 0.0;
	case PostGain:
		return 0.0;
	default:
		break;
	}

	jassertfalse;
	return 0.0f;
}

void SaturatorEffect::restoreFromValueTree(const ValueTree &v)
{
    MasterEffectProcessor::restoreFromValueTree(v);
    
	loadAttribute(Saturation, "Saturation");
	loadAttribute(WetAmount, "WetAmount");
	loadAttribute(PreGain, "PreGain");
	loadAttribute(PostGain, "PostGain");
}

ValueTree SaturatorEffect::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(Saturation, "Saturation");
	saveAttribute(WetAmount, "WetAmount");
	saveAttribute(PreGain, "PreGain");
	saveAttribute(PostGain, "PostGain");

	return v;
}

ProcessorEditorBody * SaturatorEffect::createEditor(BetterProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new SaturationEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

void SaturatorEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
    float *l = buffer.getWritePointer(0, startSample);
	float *r = buffer.getWritePointer(1, startSample);

	float const *modValues = nullptr;

	if (!saturationChain->isBypassed() && saturationChain->getNumChildProcessors() != 0)
	{
		modValues = saturationBuffer.getReadPointer(0, startSample);
	}

	for (int i = 0; i < numSamples; i++)
	{
		if (modValues != nullptr && (i & 7))
		{
			saturator.setSaturationAmount(modValues[i] * saturation);
		}

		l[i] = dry * l[i] + wet * (postGain * saturator.getSaturatedSample(preGain*l[i]));
		r[i] = dry * r[i] + wet * (postGain * saturator.getSaturatedSample(preGain*r[i]));
	}
}

void SaturatorEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate > 0)
	{
		saturationBuffer = AudioSampleBuffer(1, samplesPerBlock);
	}
}
