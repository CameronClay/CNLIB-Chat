#pragma once
#include <Windows.h>

class IOCP
{
public:
	IOCP(DWORD nThreads, DWORD nConcThreads, LPTHREAD_START_ROUTINE startAddress);
	IOCP(IOCP&& iocp);
	IOCP& operator=(IOCP&& iocp);
	IOCP(const IOCP&) = delete;
	~IOCP();

	void LinkHandle(HANDLE hnd, void* completionKey);

	HANDLE GetHandle() const;
	DWORD ThreadCount() const;
	DWORD ConcThreadCount() const;
private:
	const DWORD nThreads, nConcThreads;
	HANDLE iocp;
};