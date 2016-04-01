#pragma once
#include "CNLIB/MemPool.h"
#include "CNLIB/HeapAlloc.h"
#include "WSABufExt.h"
#include "CNLIB/BufferOptions.h"
#include "CNLIB/Socket.h"
#include "OverlappedExt.h"
#include "CNLIB/MsgHeader.h"
#include "RecvObserverI.h"

class RecvHandler
{
public:
	RecvHandler(const BufferOptions& buffOpts, UINT initialCap, RecvObserverI* observer);
	RecvHandler(RecvHandler&& recvHandler);
	~RecvHandler();

	RecvHandler& operator=(RecvHandler&& recvHandler);

	void StartRead(Socket& pc);

	bool RecvDataCR(Socket& pc, DWORD bytesTrans, const BufferOptions& buffOpts, void* obj = nullptr);
private:
	char* RecvData(DWORD& bytesTrans, char* ptr, const BufferOptions& buffOpts, void* obj);
	char* Process(char* ptr, WSABufRecv& buff, DWORD bytesToRecv, const DataHeader& header, const BufferOptions& buffOpts, void* obj);

	DWORD AppendBuffer(char* ptr, WSABufRecv& srcBuff, WSABufRecv& destBuff, DWORD& amountAppended, DWORD bytesToRecv, DWORD bytesTrans);

	WSABufRecv CreateBuffer(const BufferOptions& buffOpts, bool used);
	void SaveBuff(const WSABufRecv& buff, bool newBuff, const BufferOptions& buffOpts);
	void FreeBuffer(WSABufRecv& buff);

	MemPool<PageAllignAllocator> recvBuffPool;
	char* decompData;
	OverlappedExt ol;
	WSABufRecv primaryBuff, secondaryBuff, savedBuff;
	WSABufRecv *curBuff, *nextBuff;
	RecvObserverI* observer;
};