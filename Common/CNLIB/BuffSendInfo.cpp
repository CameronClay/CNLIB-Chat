#pragma once
#include "Stdafx.h"
#include "BuffSendInfo.h"
#include "File.h"

BuffSendInfo::BuffSendInfo()
	:
	buffer(nullptr),
	compType(NOCOMPRESSION),
	maxCompSize(0)
{}
BuffSendInfo::BuffSendInfo(CompressionType compType, DWORD nBytesDecomp, int compressionCO, DWORD maxCompSize)
	:
	buffer(nullptr),
	compType(CalcCompType(compType, nBytesDecomp, compressionCO)),
	maxCompSize((compType == SETCOMPRESSION) ? maxCompSize ? maxCompSize : FileMisc::GetCompressedBufferSize(nBytesDecomp) : 0)
{}

BuffSendInfo::BuffSendInfo(char* buffer, CompressionType compType, DWORD nBytesDecomp, int compressionCO, DWORD maxCompSize)
	:
	buffer(buffer),
	compType(CalcCompType(compType, nBytesDecomp, compressionCO)),
	maxCompSize(maxCompSize ? maxCompSize : FileMisc::GetCompressedBufferSize(nBytesDecomp))
{}

BuffSendInfo::BuffSendInfo(char* buffer, CompressionType compType, DWORD maxCompSize)
	:
	buffer(buffer),
	compType(compType),
	maxCompSize(maxCompSize)
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