#pragma once

class BuffAllocator
{
public:
	virtual char* alloc(DWORD nBytes) = 0;
	virtual void dealloc(char* data, DWORD nBytes = 0) = 0;
};