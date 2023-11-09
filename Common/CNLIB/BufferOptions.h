#pragma once
#include "Typedefs.h"

class CAMSNETLIB BufferOptions
{
public:
	BufferOptions(UINT maxDataBuffSize = 4096, UINT maxReadSize = 100000, int compression = 9, int compressionCO = 1024);
	BufferOptions& operator=(const BufferOptions& bufferOptions);

	UINT GetMaxDataBuffSize() const;
	UINT GetMaxDataCompSize() const;
	UINT GetMaxDataSize() const;
	UINT GetMaxReadSize() const;

	int GetCompression() const;
	int GetCompressionCO() const;
	int GetPageSize() const;
private:
	static DWORD CalcPageSize();
	static UINT CalcMaxDataBuffSize(DWORD pageSize, UINT maxDatBuffSize);

	const DWORD pageSize;
	const UINT maxDatBuffSize, maxDataSize, maxDatCompSize, maxReadSize; //maximum buffer size to send or recv, maximum compressed buffer size, maximum amount of data can be read or else is treated as a security threat
	const int compression, compressionCO; //compression server sends packets at
};