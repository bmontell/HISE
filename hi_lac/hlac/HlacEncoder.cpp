/*  HISE Lossless Audio Codec
*	�2017 Christoph Hart
*
*	Redistribution and use in source and binary forms, with or without modification,
*	are permitted provided that the following conditions are met:
*
*	1. Redistributions of source code must retain the above copyright notice,
*	   this list of conditions and the following disclaimer.
*
*	2. Redistributions in binary form must reproduce the above copyright notice,
*	   this list of conditions and the following disclaimer in the documentation
*	   and/or other materials provided with the distribution.
*
*	3. All advertising materials mentioning features or use of this software must
*	   display the following acknowledgement:
*	   This product includes software developed by Hart Instruments
*
*	4. Neither the name of the copyright holder nor the names of its contributors may be used
*	   to endorse or promote products derived from this software without specific prior written permission.
*
*	THIS SOFTWARE IS PROVIDED BY CHRISTOPH HART "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
*	BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*	DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
*	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

void HlacEncoder::compress(AudioSampleBuffer& source, OutputStream& output, uint32* blockOffsetData)
{
	bool compressStereo = source.getNumChannels() == 2;

	if (source.getNumSamples() == COMPRESSION_BLOCK_SIZE)
	{
		blockOffsetData[blockIndex] = numBytesWritten;
		++blockIndex;

		if (compressStereo)
		{
            auto l = CompressionHelpers::getPart(source, 0, 0, COMPRESSION_BLOCK_SIZE);
			encodeBlock(l, output);
            auto r = CompressionHelpers::getPart(source, 1, 0, COMPRESSION_BLOCK_SIZE);
			encodeBlock(r, output);
		}
		else
			encodeBlock(source, output);
		
		return;
	}

	blockOffset = 0;
	int32 numSamplesRemaining = source.getNumSamples();

	while (numSamplesRemaining >= COMPRESSION_BLOCK_SIZE)
	{
		blockOffsetData[blockIndex] = numBytesWritten;
		++blockIndex;

		uint32 numTodo = jmin<int>(COMPRESSION_BLOCK_SIZE, source.getNumSamples());

		if (compressStereo)
		{
            auto l = CompressionHelpers::getPart(source, 0, blockOffset, numTodo);
			encodeBlock(l, output);
            auto r = CompressionHelpers::getPart(source, 1, blockOffset, numTodo);
			encodeBlock(r, output);
		}
		else
		{
            auto b = CompressionHelpers::getPart(source, blockOffset, numTodo);
			encodeBlock(b, output);
		}

		
		blockOffset += numTodo;

		numSamplesRemaining -= numTodo;
	}

	if (source.getNumSamples() - blockOffset > 0)
	{
		blockOffsetData[blockIndex] = numBytesWritten;
		++blockIndex;

		const int remaining = source.getNumSamples() - blockOffset;

		if (compressStereo)
		{
            auto l = CompressionHelpers::getPart(source, 0, blockOffset, remaining);
			encodeLastBlock(l, output);
            auto r = CompressionHelpers::getPart(source, 1, blockOffset, remaining);
			encodeLastBlock(r, output);
		}
		else
        {
            auto b = CompressionHelpers::getPart(source, blockOffset, remaining);
            encodeLastBlock(b, output);
        }
			
	}
}

void HlacEncoder::reset()
{
	indexInBlock = 0;
	currentCycle = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	workBuffer = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	blockOffset = 0;
	blockIndex = 0;
	numBytesWritten = 0;
	numBytesUncompressed = 0;
	numTemplates = 0;
	numDeltas = 0;
	blockOffset = 0;
	bitRateForCurrentCycle = 0;
	firstCycleLength = -1;
	ratio = 0.0f;
}


float HlacEncoder::getCompressionRatio() const
{
	return (float)(numBytesWritten) / (float)(numBytesUncompressed);
}

bool HlacEncoder::encodeBlock(AudioSampleBuffer& block, OutputStream& output)
{
	auto block16 = CompressionHelpers::AudioBufferInt16(block, false);

	return encodeBlock(block16, output);
}

bool HlacEncoder::encodeBlock(CompressionHelpers::AudioBufferInt16& block16, OutputStream& output)
{
	jassert(block16.size == COMPRESSION_BLOCK_SIZE);

	firstCycleLength = -1;

	auto maxBitDepth = CompressionHelpers::getPossibleBitReductionAmount(block16);

	LOG("ENC " + String(numBytesUncompressed/2) + "\t\tNew Block with bit depth: " + String(maxBitDepth));

	numBytesUncompressed += COMPRESSION_BLOCK_SIZE * 2;

	if (maxBitDepth <= options.bitRateForWholeBlock)
	{
		indexInBlock = 0;
		return encodeCycle(block16, output);
	}

	indexInBlock = 0;

	while (!isBlockExhausted())
	{
        int numRemaining = COMPRESSION_BLOCK_SIZE - indexInBlock;
		auto rest = CompressionHelpers::getPart(block16, indexInBlock, numRemaining);

		if (numRemaining <= 4)
		{
			if (!encodeCycleDelta(rest, output))
				return false;

			indexInBlock += numRemaining;

			continue;
		}

		int idealCycleLength;

		if (firstCycleLength < 0)
		{
			if (options.fixedBlockWidth > 0)
				idealCycleLength = options.fixedBlockWidth;
			else
				idealCycleLength = getCycleLength(rest) + 1;

			if (options.reuseFirstCycleLengthForBlock)
				firstCycleLength = idealCycleLength;
		}
		else
			idealCycleLength = firstCycleLength;

		int cycleLength = idealCycleLength == 0 ? numRemaining : jmin<int>(numRemaining, idealCycleLength);

		currentCycle = CompressionHelpers::getPart(rest, 0, cycleLength);

		bitRateForCurrentCycle = CompressionHelpers::getPossibleBitReductionAmount(currentCycle);

		if (options.useDiffEncodingWithFixedBlocks)
		{
			auto byteAmount = CompressionHelpers::getByteAmountForDifferential(currentCycle);
			auto normalByteAmount = collection.getNumBytesForBitRate(bitRateForCurrentCycle, cycleLength);

			indexInBlock += cycleLength;

			if (byteAmount == normalByteAmount)
				encodeCycle(currentCycle, output);
			else
				encodeDiff(currentCycle, output);

			continue;
		}

		indexInBlock += cycleLength;

		if (!encodeCycle(currentCycle, output))
			return false;


		while (options.useDeltaEncoding && !isBlockExhausted())
		{
			if (numRemaining > 2 * cycleLength)
			{
				auto nextCycle = CompressionHelpers::getPart(block16, indexInBlock, cycleLength);
				uint8 bitReductionDelta = CompressionHelpers::getBitReductionWithTemplate(currentCycle, nextCycle, options.removeDcOffset);

				//DBG("Bit rate reduction for delta storage: " + String(bitReductionDelta));

				//LOG("        Index: " + String(indexInBlock) + " BitReduction: " + String(bitReductionDelta));

				float factor = (float)bitReductionDelta / (float)bitRateForCurrentCycle;

				if (factor > options.deltaCycleThreshhold)
				{
					int nextCycleSize = getCycleLengthFromTemplate(nextCycle, rest);
					nextCycle.size = nextCycleSize;

					indexInBlock += nextCycle.size;
                    
                    numRemaining = COMPRESSION_BLOCK_SIZE - indexInBlock;
                    
                    jassert(indexInBlock <= COMPRESSION_BLOCK_SIZE);

					encodeCycleDelta(nextCycle, output);

				}
				else
					break;

			}
			else
				break;
		}
	}

	return true;
}

uint8 HlacEncoder::getBitReductionAmountForMSEncoding(AudioSampleBuffer& block)
{
	ignoreUnused(block);

	return 0;

#if 0
	if (block.getNumChannels() == 1)
		return 0;

	float** channels = block.getArrayOfWritePointers();

	AudioSampleBuffer lb(channels, 1, block.getNumSamples());
	AudioSampleBuffer lr(channels + 1, 1, block.getNumSamples());

	auto l = CompressionHelpers::AudioBufferInt16(lb, false);
	auto r = CompressionHelpers::AudioBufferInt16(lr, false);

	auto bdl = CompressionHelpers::getPossibleBitReductionAmount(l);
	auto bdr = CompressionHelpers::getPossibleBitReductionAmount(r);

	CompressionHelpers::AudioBufferInt16 workBuffer(block.getNumSamples());

	CompressionHelpers::IntVectorOperations::sub(workBuffer.getWritePointer(), l.getReadPointer(), r.getReadPointer(), block.getNumSamples());

	auto bds = CompressionHelpers::getPossibleBitReductionAmount(workBuffer);

	if (bds < bdr)
	{
		return bdr - bds;
	}

	return 0;
#endif
}

bool HlacEncoder::encodeCycle(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output)
{
	if (cycle.size == 0)
		return true;

	++numTemplates;

	auto compressor = collection.getSuitableCompressorForData(cycle.getReadPointer(), cycle.size);

	auto numBytesToWrite = compressor->getByteAmount(cycle.size);
    
	LOG("ENC  " + String(blockOffset + indexInBlock-cycle.size) + "\t\t\tNew Template with bit depth " + String(compressor->getAllowedBitRange()) + ": " + String(cycle.size));

	if (!writeCycleHeader(true, compressor->getAllowedBitRange(), cycle.size, output))
		return false;

	if (numBytesToWrite > 0)
	{
		numBytesWritten += numBytesToWrite;

		MemoryBlock mb;
		mb.setSize(numBytesToWrite, true);
		compressor->compress((uint8*)mb.getData(), cycle.getReadPointer(), cycle.size);

		bool checkDecompression = false;

		if (checkDecompression)
		{
			CompressionHelpers::AudioBufferInt16 shouldBeZero(cycle.size);

			memcpy(shouldBeZero.getWritePointer(), cycle.getReadPointer(), sizeof(int16)*cycle.size);


			compressor->decompress(shouldBeZero.getWritePointer(), (uint8*)mb.getData(), cycle.size);

			CompressionHelpers::IntVectorOperations::sub(shouldBeZero.getWritePointer(), cycle.getReadPointer(), cycle.size);
			
			jassert(CompressionHelpers::getPossibleBitReductionAmount(shouldBeZero) == 0);

		}


		return output.write(mb.getData(), numBytesToWrite);
	}

	return true;
}


bool HlacEncoder::encodeDiff(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output)
{
	jassert(cycle.size % 4 == 0);

	int numFullValues = CompressionHelpers::Diff::getNumFullValues(cycle.size);

	auto packedBuffer = CompressionHelpers::Diff::createBufferWithFullValues(cycle);

	jassert(packedBuffer.size == numFullValues);

	auto compressorFull = collection.getSuitableCompressorForData(packedBuffer.getReadPointer(), packedBuffer.size);
	auto numBytesForFull = compressorFull->getByteAmount(numFullValues);
	auto bitRateFull = compressorFull->getAllowedBitRange();

	auto numErrorValues = CompressionHelpers::Diff::getNumErrorValues(cycle.size);
	auto packedErrorBuffer = CompressionHelpers::Diff::createBufferWithErrorValues(cycle, packedBuffer);

	auto compressorError = collection.getSuitableCompressorForData(packedErrorBuffer.getReadPointer(), packedErrorBuffer.size);
	auto numBytesForError = compressorError->getByteAmount(numErrorValues);
	auto bitRateError = compressorError->getAllowedBitRange();



	if (!writeDiffHeader(bitRateFull, bitRateError, cycle.size, output))
		return false;

	MemoryBlock mbFull;
	mbFull.setSize(numBytesForFull);
	compressorFull->compress((uint8*)mbFull.getData(), packedBuffer.getReadPointer(), numFullValues);

	if (!output.write(mbFull.getData(), numBytesForFull))
		return false;

	numBytesWritten += numBytesForFull;

	MemoryBlock mbError;
	mbError.setSize(numBytesForError);
	compressorError->compress((uint8*)mbError.getData(), packedErrorBuffer.getReadPointer(), numErrorValues);

	if (!output.write(mbError.getData(), numBytesForError))
		return false;

	numBytesWritten += numBytesForError;

	LOG("ENC  " + String(blockOffset + indexInBlock) + "\t\t\tNew Diff block bit depth " + String(compressorFull->getAllowedBitRange()) + " -> " + String(compressorError->getAllowedBitRange()) + ": " + String(cycle.size));

	return true;
}

bool HlacEncoder::encodeCycleDelta(CompressionHelpers::AudioBufferInt16& nextCycle, OutputStream& output)
{
    if(nextCycle.size < 8)
    {
        return encodeCycle(nextCycle, output);
    }
    
	++numDeltas;

	//CompressionHelpers::AudioBufferInt16 workBuffer(nextCycle.size);
	CompressionHelpers::IntVectorOperations::sub(workBuffer.getWritePointer(), nextCycle.getReadPointer(), currentCycle.getReadPointer(), nextCycle.size);
	auto compressor = collection.getSuitableCompressorForData(workBuffer.getReadPointer(), nextCycle.size);
	auto numBytesToWrite = compressor->getByteAmount(nextCycle.size);

	LOG("ENC   " + String(blockOffset + indexInBlock) + "\t\t\t\tSave delta with bit rate " + String(compressor->getAllowedBitRange()) + ": " + String(nextCycle.size));

	if (!writeCycleHeader(false, compressor->getAllowedBitRange(), nextCycle.size, output))
		return false;

	numBytesWritten += numBytesToWrite;

	MemoryBlock mb;
	mb.setSize(numBytesToWrite, true);
	compressor->compress((uint8*)mb.getData(), workBuffer.getReadPointer(), nextCycle.size);
	return output.write(mb.getData(), numBytesToWrite);
}

bool HlacEncoder::writeCycleHeader(bool isTemplate, int bitDepth, int numSamples, OutputStream& output)
{
	if (bitDepth == 0)
	{
		if (!output.writeByte(1))
			return false;
	}
	else if (bitDepth == 1)
	{
		if (!output.writeByte(3))
			return false;
	}
	else if (bitDepth == 2)
	{
		if (!output.writeByte(5))
			return false;
	}
	else
	{
		uint8 header = isTemplate ? 1 : 0;

		jassert(numSamples <= COMPRESSION_BLOCK_SIZE);

		uint8 bitDepthMoved = (uint8)(((bitDepth - 1) & 15) << 1);
		header |= bitDepthMoved;

		jassert(header != 0xE0); // Reserved for diff

		if (!output.writeByte(header))
			return false;
	}

	numBytesWritten += 3;

	return output.writeShort((int16)numSamples);
}


bool HlacEncoder::writeDiffHeader(int fullBitRate, int errorBitRate, int blockSize, OutputStream& output)
{
	if (!output.writeByte((uint8)0xE0))
		return false;

	uint8 bitRates = (uint8)(fullBitRate - 1) << 4;
	bitRates |= (uint8)(errorBitRate - 1);

	uint8 blockSizeLog = (uint8)log2(blockSize);

	uint16 shortPacked = bitRates << 8;
	shortPacked |= blockSizeLog;

	output.writeShort(shortPacked);

	numBytesWritten += 3;

	return true;
}


void HlacEncoder::encodeLastBlock(AudioSampleBuffer& block, OutputStream& output)
{
	CompressionHelpers::AudioBufferInt16 a(block, false);

	encodeCycle(a, output);

	int numZerosToPad = COMPRESSION_BLOCK_SIZE - a.size;

	jassert(numZerosToPad > 0);

	LOG("ENC  PADDING " + String(numZerosToPad));

	writeCycleHeader(true, 0, numZerosToPad, output);

}


/** Returns the best length for the cycle template.
*
* It calculates the perfect length as fractional number and then returns the upper ceiling value.
* This is so that subsequent cycles can use the additional sample or not depending on the bit reduction amount.
*
* If it can't reduce the bit range it will return the block size
*/
int HlacEncoder::getCycleLength(CompressionHelpers::AudioBufferInt16& block)
{
	int unused;
	return CompressionHelpers::getCycleLengthWithLowestBitRate(block, unused, workBuffer);
}

int HlacEncoder::getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest)
{
	auto candidate1 = CompressionHelpers::getPart(rest, newCycle.size - 1, newCycle.size);
	auto candidate2 = CompressionHelpers::getPart(rest, newCycle.size - 2, newCycle.size);

	auto br1 = CompressionHelpers::getBitReductionWithTemplate(currentCycle, candidate1, options.removeDcOffset);
	auto br2 = CompressionHelpers::getBitReductionWithTemplate(currentCycle, candidate2, options.removeDcOffset);

	if (br1 > br2)
	{
		return newCycle.size - 1;
	}
	else
	{
		return newCycle.size;
	}
}