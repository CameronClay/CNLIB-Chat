#pragma once
#include "Typedefs.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/BufferOptions.h"
#include "CNLIB/BuffAllocator.h"

class CAMSNETLIB StreamAllocInterface
{
public:
	virtual char* GetSendBuffer() = 0; 
	virtual char* GetSendBuffer(BuffAllocator* alloc, DWORD nBytes) = 0;

	virtual MsgStreamWriter CreateOutStream(short type, short msg) = 0;
	virtual MsgStreamWriter CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg) = 0;

	virtual const BufferOptions GetBufferOptions() const = 0;
};