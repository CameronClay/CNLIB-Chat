#pragma once
#include "Typedefs.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/BufferOptions.h"

class CAMSNETLIB StreamAllocInterface
{
public:
	virtual char* GetSendBuffer() = 0;
	virtual MsgStreamWriter CreateOutStream(short type, short msg) = 0;
	virtual const BufferOptions GetBufferOptions() const = 0;
};