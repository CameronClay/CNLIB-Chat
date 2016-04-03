#include "StdAfx.h"
#include "CNLIB/BufferOptions.h"
#include "CNLIB/File.h"
#include "CNLIB/MsgHeader.h"

BufferOptions::BufferOptions(UINT maxDataBuffSize, UINT maxReadSize, int compression, int compressionCO)
	:
	pageSize(CalcPageSize()),
	maxDatBuffSize(CalcMaxDataBuffSize(pageSize, maxDataBuffSize)), //sizeof(size_t) for Element*
	maxDataSize(maxDatBuffSize - sizeof(DataHeader)),
	maxDatCompSize(FileMisc::GetCompressedBufferSize(maxDataSize)),
	maxReadSize(maxReadSize - sizeof(DataHeader)),
	compression(compression),
	compressionCO(compressionCO)
{}

BufferOptions& BufferOptions::operator=(const BufferOptions& bufferOptions)
{
	if (this != &bufferOptions)
	{
		const_cast<DWORD&>(pageSize) = bufferOptions.pageSize;
		const_cast<UINT&>(maxDatBuffSize) = bufferOptions.maxDatBuffSize;
		const_cast<UINT&>(maxDataSize) = bufferOptions.maxDataSize;
		const_cast<UINT&>(maxDatCompSize) = bufferOptions.maxDatCompSize;
		const_cast<int&>(compression) = bufferOptions.compression;
		const_cast<int&>(compressionCO) = bufferOptions.compressionCO;
	}
	return *this;
}

UINT BufferOptions::GetMaxDataBuffSize() const
{
	return maxDatBuffSize;
}
UINT BufferOptions::GetMaxDataCompSize() const
{
	return maxDatCompSize;
}
UINT BufferOptions::GetMaxDataSize() const
{
	return maxDataSize;
}
UINT BufferOptions::GetMaxReadSize() const
{
	return maxReadSize;
}

int BufferOptions::GetCompression() const
{
	return compression;
}
int BufferOptions::GetCompressionCO() const
{
	return compressionCO;
}

int BufferOptions::GetPageSize() const
{
	return pageSize;
}

DWORD BufferOptions::CalcPageSize()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwPageSize;
}

UINT BufferOptions::CalcMaxDataBuffSize(DWORD pageSize, UINT maxDataBuffSize)
{
	const UINT roundTo = pageSize - sizeof(size_t);
	const UINT roundFrom = maxDataBuffSize - sizeof(size_t);
	return ((roundFrom + roundTo - 1) / roundTo) * roundTo;
}