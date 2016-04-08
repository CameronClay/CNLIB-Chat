//Copyright (c) <2015> <Cameron Clay>

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