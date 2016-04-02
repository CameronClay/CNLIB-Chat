#pragma once
#include "Typedefs.h"

class CAMSNETLIB BufferOptions
{
public:
	BufferOptions(UINT maxDataBuffSize = 4096, int compression = 9, int compressionCO = 512);
	BufferOptions& operator=(const BufferOptions& bufferOptions);

	UINT GetMaxDataBuffSize() const;
	UINT GetMaxDataCompSize() const;
	UINT GetMaxDataSize() const;

	int GetCompression() const;
	int GetCompressionCO() const;
	int GetPageSize() const;
private:
	static DWORD CalcPageSize();
	static UINT CalcMaxDataBuffSize(DWORD pageSize, UINT maxDatBuffSize);

	const DWORD pageSize;
	const UINT maxDatBuffSize, maxDataSize, maxDatCompSize; //maximum buffer size to send or recv, maximum compressed buffer size
	const int compression, compressionCO; //compression server sends packets at
};