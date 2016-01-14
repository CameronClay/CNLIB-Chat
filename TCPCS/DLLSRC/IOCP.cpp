#include "IOCP.h"

IOCP::IOCP(DWORD nThreads, DWORD nConcThreads, LPTHREAD_START_ROUTINE startAddress)
	:
	nThreads(nThreads),
	nConcThreads(nConcThreads),
	iocp(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, nConcThreads))
{
	for (int i = 0; i < nThreads; i++)
	{
		CloseHandle(CreateThread(NULL, 0, startAddress, this, NULL, NULL));
	}
}

IOCP::IOCP(IOCP&& iocp)
	:
	nThreads(iocp.nThreads),
	nConcThreads(iocp.nConcThreads),
	iocp(iocp.iocp)
{
	memset(&iocp, 0, sizeof(IOCP));
}

IOCP& IOCP::operator=(IOCP&& iocp)
{
	if (this != &iocp)
	{
		const_cast<DWORD&>(nThreads) = iocp.nThreads;
		const_cast<DWORD&>(nConcThreads) = iocp.nConcThreads;
		this->iocp = iocp.iocp;

		ZeroMemory(&iocp, sizeof(IOCP));
	}
	return *this;
}

IOCP::~IOCP()
{
	if (iocp)
	{
		CloseHandle(iocp);
		iocp = NULL;
	}
}

void IOCP::LinkHandle(HANDLE hnd, void* completionKey)
{
	CreateIoCompletionPort(hnd, iocp, (ULONG_PTR)completionKey, NULL);
}

HANDLE IOCP::GetHandle() const
{
	return iocp;
}
DWORD IOCP::ThreadCount() const
{
	return nThreads;
}
DWORD IOCP::ConcThreadCount() const
{
	return nConcThreads;
}