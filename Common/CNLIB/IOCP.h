#pragma once

class IOCP
{
public:
	IOCP(DWORD nThreads, DWORD nConcThreads, LPTHREAD_START_ROUTINE startAddress);
	IOCP(IOCP&& iocp);
	IOCP(const IOCP&) = delete;
	IOCP& operator=(IOCP&& iocp);

	void WaitAndCleanup();

	bool LinkHandle(HANDLE hnd, void* completionKey);

	void Post(DWORD bytesTrans, void* completionKey, OVERLAPPED* ol = nullptr);

	HANDLE GetHandle() const;
	DWORD ThreadCount() const;
	DWORD ConcThreadCount() const;
private:
	const DWORD nThreads, nConcThreads;
	HANDLE* threads;
	HANDLE iocp;
};