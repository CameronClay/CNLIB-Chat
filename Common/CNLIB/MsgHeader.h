#pragma once
#include "BufSize.h"

struct DataHeader
{
	BufSize size;
	unsigned short index;
};

struct MsgHeader : public DataHeader
{
	short type, msg;
};