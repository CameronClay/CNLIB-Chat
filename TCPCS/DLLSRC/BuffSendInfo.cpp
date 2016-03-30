#pragma once
#include "Stdafx.h"
#include "CNLIB/BuffSendInfo.h"
#include "CNLIB/File.h"

BuffSendInfo::BuffSendInfo()
	:
	buffer(nullptr),
	compType(NOCOMPRESSION),
	maxCompSize(0),
	alloc(nullptr)
{}
BuffSendInfo::BuffSendInfo(CompressionType compType, DWORD nBytesDecomp, int compressionCO, DWORD maxCompSize)
	:
	buffer(nullptr),
	compType(CalcCompType(compType, nBytesDecomp, compressionCO)),
	maxCompSize((this->compType == SETCOMPRESSION) ? (maxCompSize ? maxCompSize : FileMisc::GetCompressedBufferSize(nBytesDecomp)) : 0),
	alloc(nullptr)
{}

BuffSendInfo::BuffSendInfo(char* buffer, CompressionType compType, DWORD nBytesDecomp, int compressionCO, DWORD maxCompSize)
	:
	buffer(buffer),
	compType(CalcCompType(compType, nBytesDecomp, compressionCO)),
	maxCompSize(maxCompSize ? maxCompSize : FileMisc::GetCompressedBufferSize(nBytesDecomp)),
	alloc(nullptr)
{}

BuffSendInfo::BuffSendInfo(char* buffer, CompressionType compType, DWORD maxCompSize)
	:
	buffer(buffer),
	compType(compType),
	maxCompSize(maxCompSize),
	alloc(nullptr)
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