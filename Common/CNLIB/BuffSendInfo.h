#pragma once
#include "Typedefs.h"
#include "CNLIB/CompressionTypes.h"

struct CAMSNETLIB BuffSendInfo
{
	BuffSendInfo(CompressionType compType, DWORD nBytesDecomp, int compressionCO);
	BuffSendInfo(char* buffer, CompressionType compType, DWORD nBytesDecomp, int compressionCO);
	BuffSendInfo(char* buffer, CompressionType compType);
	static CompressionType CalcCompType(CompressionType compType, DWORD nBytesDecomp, int compressionCO);

	char* buffer;
	CompressionType compType;
};