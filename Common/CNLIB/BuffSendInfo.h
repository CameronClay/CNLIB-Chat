#pragma once
#include "Typedefs.h"
#include "CNLIB/CompressionTypes.h"
#include "CNLIB/BuffAllocator.h"

struct CAMSNETLIB BuffSendInfo
{
	BuffSendInfo();
	BuffSendInfo(CompressionType compType, DWORD nBytesDecomp, int compressionCO, DWORD maxCompSize = 0);
	BuffSendInfo(char* buffer, CompressionType compType, DWORD nBytesDecomp, int compressionCO, DWORD maxCompSize = 0);
	BuffSendInfo(char* buffer, CompressionType compType, DWORD maxCompSize = 0);
	static CompressionType CalcCompType(CompressionType compType, DWORD nBytesDecomp, int compressionCO);

	char* buffer;
	const CompressionType compType;
	const DWORD maxCompSize;
	BuffAllocator* alloc;
};