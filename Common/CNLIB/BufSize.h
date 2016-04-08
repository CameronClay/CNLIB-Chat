//Copyright (c) <2015> <Cameron Clay>

#pragma once

union BufSize
{
	BufSize(DWORD64 size = 0ull)
		:
		size(size)
	{}

	struct
	{
		DWORD nBytesComp, nBytesDecomp;
	} up;
	DWORD64 size;
};
