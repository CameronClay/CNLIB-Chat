#include "StdAfx.h"
#include "CNLIB/BufferOptions.h"
#include "CNLIB/File.h"

BufferOptions::BufferOptions(UINT maxDataSize, int compression, int compressionCO)
	:
	maxDataSize(maxDataSize),
	maxCompSize(FileMisc::GetCompressedBufferSize(maxDataSize)),
	compression(compression),
	compressionCO(compressionCO)
{}

BufferOptions& BufferOptions::operator=(const BufferOptions& bufferOptions)
{
	if (this != &bufferOptions)
	{
		const_cast<UINT&>(maxDataSize) = bufferOptions.maxDataSize;
		const_cast<UINT&>(maxCompSize) = bufferOptions.maxCompSize;
		const_cast<int&>(compression) = bufferOptions.compression;
		const_cast<int&>(compressionCO) = bufferOptions.compressionCO;
	}
	return *this;
}

UINT BufferOptions::GetMaxDataSize() const
{
	return maxDataSize;
}
UINT BufferOptions::GetMaxCompSize() const
{
	return maxCompSize;
}
int BufferOptions::GetCompression() const
{
	return compression;
}
int BufferOptions::GetCompressionCO() const
{
	return compressionCO;
}