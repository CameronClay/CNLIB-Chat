#pragma once
#include "CNLIB/BufferOptions.h"
#include "CNLIB/StreamAllocInterface.h"
#include "CNLIB/WSABufExt.h"
#include "CNLIB/CompressionTypes.h"
#include "CNLIB/MemPool.h"
#include "CNLIB/HeapAlloc.h"
#include "CNLIB/BuffAllocator.h"

class DataPoolObserver : public BuffAllocator
{
public:
	DataPoolObserver(const BufferOptions& bufferOptions, UINT maxDataSize = 4096, UINT sendBuffCount = 40);
	DataPoolObserver(DataPoolObserver&& dataPoolObs);
	DataPoolObserver& operator=(DataPoolObserver&& dataPoolObs);

	void dealloc(char* data, DWORD nBytes = 0) override;
	char* alloc(DWORD = NULL) override;
private:
	MemPoolSync<PageAllignAllocator> sendDataPool; //Used to help speed up allocation of send buffers
};

class MsgPoolObserver : public BuffAllocator
{
public:
	MsgPoolObserver(UINT sendMsgBuffCount = 20);
	MsgPoolObserver(MsgPoolObserver&& msgPoolObs);
	MsgPoolObserver& operator=(MsgPoolObserver&& msgPoolObs);

	void dealloc(char* data, DWORD nBytes = 0) override;
	char* alloc(DWORD = NULL) override;
private:
	MemPoolSync<HeapAllocator> sendMsgPool; //Used to help speed up allocation of send buffers
};

class BufSendAlloc : public StreamAllocInterface
{
public:
	BufSendAlloc(UINT maxDataSize = 4096, UINT sendBuffCount = 40, UINT sendMsgBuffCount = 20, int compression = 9, int compressionCO = 512);
	BufSendAlloc(const BufSendAlloc&) = delete;
	BufSendAlloc(BufSendAlloc&& bufSendAlloc);
	BufSendAlloc& operator=(BufSendAlloc&& bufSendAlloc);

	char* GetSendBuffer() override;
	char* GetSendBuffer(BuffAllocator* alloc, DWORD nBytes) override;

	MsgStreamWriter CreateOutStream(short type, short msg) override;
	MsgStreamWriter CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg) override;

	const BufferOptions GetBufferOptions() const override;

	WSABufSend CreateBuff(DWORD nBytesDecomp, char* buffer, bool msg, USHORT index = -1, CompressionType compType = BESTFIT, BuffAllocator* alloc = nullptr);
	void FreeBuff(WSABufSend& buff);
private:
	const BufferOptions bufferOptions;
	std::atomic<USHORT> bufIndex;
	DataPoolObserver dataPool;
	MsgPoolObserver msgPool;
};