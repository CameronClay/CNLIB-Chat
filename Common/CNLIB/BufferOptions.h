#pragma once
#include "Typedefs.h"

class CAMSNETLIB BufferOptions
{
public:
	BufferOptions(UINT maxDataSize = 8192, int compression = 9, int compressionCO = 512);
	BufferOptions& operator=(const BufferOptions& bufferOptions);

	UINT GetMaxDatBuffSize() const;
	UINT GetMaxDatCompSize() const;
	UINT GetMaxDataSize() const;

	int GetCompression() const;
	int GetCompressionCO() const;
	int GetPageSize() const;
private:
	static DWORD CalcPageSize();

	const DWORD pageSize;
	const UINT maxDatBuffSize, maxDatCompSize, maxDataSize; //maximum buffer size to send or recv, maximum compressed buffer size
	const int compression, compressionCO; //compression server sends packets at
};