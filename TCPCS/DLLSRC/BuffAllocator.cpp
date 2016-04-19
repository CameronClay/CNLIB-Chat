#include "StdAfx.h"
#include "CNLIB/BuffAllocator.h"
#include "CNLIB/HeapAlloc.h"

BuffHeapAllocator::BuffHeapAllocator(){}

char* BuffHeapAllocator::alloc(DWORD nBytes)
{
	return ::alloc<char>(nBytes);
}

void BuffHeapAllocator::dealloc(char* data, DWORD nBytes)
{
	::dealloc(data);
}