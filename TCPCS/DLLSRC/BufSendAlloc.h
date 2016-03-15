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
	BufSendAlloc(UINT maxDataSize = 8192, UINT sendBuffCount = 40, UINT sendMsgBuffCount = 20, int compression = 9, int compressionCO = 512);
	BufSendAlloc(const BufSendAlloc&) = delete;
	BufSendAlloc(BufSendAlloc&& bufSendAlloc);
	BufSendAlloc& operator=(BufSendAlloc&& bufSendAlloc);

	char* GetSendBuffer() override;
	MsgStreamWriter CreateOutStream(short type, short msg) override;
	const BufferOptions GetBufferOptions() const override;

	WSABufSend CreateBuff(DWORD nBytesDecomp, char* buffer, bool msg, CompressionType compType = BESTFIT);
	void FreeBuff(WSABufSend& buff);
private:
	const BufferOptions bufferOptions;
	MemPoolSync<HeapAllocator> sendDataPool, sendMsgPool; //Used to help speed up allocation of send buffers
};