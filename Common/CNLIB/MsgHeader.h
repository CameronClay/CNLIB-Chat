#pragma once
#include "BufSize.h"

struct DataHeader
{
	BufSize size;
};

struct MsgHeader : public DataHeader
{
	short type, msg;
};