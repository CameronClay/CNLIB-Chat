#pragma once
#include "CNLIB/BufferOptions.h"
#include "CNLIB/StreamAllocInterface.h"
#include "CNLIB/WSABufExt.h"
#include "CNLIB/CompressionTypes.h"
#include "CNLIB/MemPool.h"
#include "CNLIB/HeapAlloc.h"

class BufSendAlloc : public StreamAllocInterface
{
public:
	BufSendAlloc(UINT maxDataSize = 4096, UINT sendBuffCount = 40, UINT sendMsgBuffCount = 20, int compression = 9, int compressionCO = 512);
	BufSendAlloc(const BufSendAlloc&) = delete;
	BufSendAlloc(BufSendAlloc&& bufSendAlloc);
	BufSendAlloc& operator=(BufSendAlloc&& bufSendAlloc);

	char* GetSendBuffer() override;
	MsgStreamWriter CreateOutStream(short type, short msg) override;
	//MsgStreamWriter CreateOutStream(char* data, DWORD nBytes, short type, short msg) override;
	const BufferOptions GetBufferOptions() const override;

	WSABufExt CreateBuff(DWORD nBytesDecomp, char* buffer, bool msg, USHORT index = -1, CompressionType compType = BESTFIT);
	void FreeBuff(WSABufExt& buff);
private:
	const BufferOptions bufferOptions;
	std::atomic<USHORT> bufIndex;
	MemPoolSync<PageAllignAllocator> sendDataPool; //Used to help speed up allocation of send buffers
	MemPoolSync<HeapAllocator> sendMsgPool; //Used to help speed up allocation of send buffers
};