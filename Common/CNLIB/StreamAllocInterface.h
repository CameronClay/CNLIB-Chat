//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Typedefs.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/BufferOptions.h"
#include "CNLIB/BuffAllocator.h"
#include "CNLIB/BuffSendInfo.h"

class CAMSNETLIB StreamAllocInterface
{
public:
	virtual BuffSendInfo GetSendBuffer(DWORD hiByteEstimate, CompressionType compType = BESTFIT) = 0;
	virtual BuffSendInfo GetSendBuffer(BuffAllocator* alloc, DWORD nBytes, CompressionType compType = BESTFIT) = 0;

	virtual MsgStreamWriter CreateOutStream(DWORD hiByteEstimate, short type, short msg, CompressionType compType = BESTFIT) = 0;
	virtual MsgStreamWriter CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg, CompressionType compType = BESTFIT) = 0;

	virtual const BufferOptions GetBufferOptions() const = 0;
};