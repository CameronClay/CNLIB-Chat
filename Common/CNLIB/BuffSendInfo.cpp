#pragma once
#include "Stdafx.h"
#include "BuffSendInfo.h"

BuffSendInfo::BuffSendInfo(CompressionType compType, DWORD nBytesDecomp, int compressionCO)
	:
	buffer(nullptr),
	compType(CalcCompType(compType, nBytesDecomp, compressionCO))
{}

BuffSendInfo::BuffSendInfo(char* buffer, CompressionType compType, DWORD nBytesDecomp, int compressionCO)
	:
	buffer(buffer),
	compType(CalcCompType(compType, nBytesDecomp, compressionCO))
{}

BuffSendInfo::BuffSendInfo(char* buffer, CompressionType compType)
	:
	buffer(buffer),
	compType(compType)
{}

CompressionType BuffSendInfo::CalcCompType(CompressionType compType, DWORD nBytesDecomp, int compressionCO)
{
	if (compType == BESTFIT)
	{
		if (nBytesDecomp >= compressionCO)
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}
	return compType;
}