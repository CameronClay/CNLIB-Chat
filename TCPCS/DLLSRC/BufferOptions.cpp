#include "StdAfx.h"
#include "CNLIB/BufferOptions.h"
#include "CNLIB/File.h"
#include "CNLIB/MsgHeader.h"

BufferOptions::BufferOptions(UINT maxDataSize, int compression, int compressionCO)
	:
	pageSize(CalcPageSize()),
	maxDatBuffSize(((maxDataSize + pageSize - 1) / pageSize) * pageSize),
	maxDatCompSize(FileMisc::GetCompressedBufferSize(maxDataSize)),
	maxDataSize(maxDatBuffSize - sizeof(DataHeader)),
	compression(compression),
	compressionCO(compressionCO)
{}

BufferOptions& BufferOptions::operator=(const BufferOptions& bufferOptions)
{
	if (this != &bufferOptions)
	{
		const_cast<DWORD&>(pageSize) = bufferOptions.pageSize;
		const_cast<UINT&>(maxDatBuffSize) = bufferOptions.maxDatBuffSize;
		const_cast<UINT&>(maxDatCompSize) = bufferOptions.maxDatCompSize;
		const_cast<UINT&>(maxDataSize) = bufferOptions.maxDataSize;
		const_cast<int&>(compression) = bufferOptions.compression;
		const_cast<int&>(compressionCO) = bufferOptions.compressionCO;
	}
	return *this;
}

UINT BufferOptions::GetMaxDatBuffSize() const
{
	return maxDatBuffSize;
}
UINT BufferOptions::GetMaxDatCompSize() const
{
	return maxDatCompSize;
}
UINT BufferOptions::GetMaxDataSize() const
{
	return maxDataSize;
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