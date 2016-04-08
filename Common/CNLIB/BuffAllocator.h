//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Typedefs.h"

class BuffAllocator
{
public:
	virtual char* alloc(DWORD nBytes) = 0;
	virtual void dealloc(char* data, DWORD nBytes = 0) = 0;
};

class CAMSNETLIB BuffHeapAllocator : public BuffAllocator
{
public:
	BuffHeapAllocator();
	char* alloc(DWORD nBytes) override;
	void dealloc(char* data, DWORD nBytes = 0) override;
};