#pragma once
#include "Typedefs.h"

class CAMSNETLIB BufferOptions
{
public:
	BufferOptions(UINT maxDataSize = 8192, int compression = 9, int compressionCO = 512);
	BufferOptions& operator=(const BufferOptions& bufferOptions);

	UINT GetMaxDataSize() const;
	UINT GetMaxCompSize() const;
	int GetCompression() const;
	int GetCompressionCO() const;
	int GetPageSize() const;
private:
	static DWORD CalcPageSize();

	const UINT maxDataSize, maxCompSize; //maximum packet size to send or recv, maximum compressed data size, number of preallocated sendbuffers
	const int compression, compressionCO; //compression server sends packets at
	const DWORD pageSize;
};