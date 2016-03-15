#include "StdAfx.h"
#include "BufSendAlloc.h"
#include "CNLIB/File.h"
#include "CNLIB/MsgStream.h"

BufSendAlloc::BufSendAlloc(UINT maxDataSize, UINT sendBuffCount, UINT sendMsgBuffCount, int compression, int compressionCO)
	:
	bufferOptions(maxDataSize, compression, compressionCO),
	sendDataPool(maxDataSize + bufferOptions.GetMaxCompSize() + sizeof(DWORD64) * 2, sendBuffCount), //extra DWORD64 incase it sends compressed data, because data written to initial buffer is offset by sizeof(DWORD64)
	sendMsgPool(sizeof(DWORD64) + MSG_OFFSET, sendMsgBuffCount)
{}
BufSendAlloc::BufSendAlloc(BufSendAlloc&& bufSendAlloc)
	:
	bufferOptions(bufSendAlloc.bufferOptions),
	sendDataPool(std::move(bufSendAlloc.sendDataPool)),
	sendMsgPool(std::move(bufSendAlloc.sendMsgPool))
{
	memset(&bufSendAlloc, 0, sizeof(BufSendAlloc));
}
BufSendAlloc& BufSendAlloc::operator=(BufSendAlloc&& bufSendAlloc)
{
	if (this != &bufSendAlloc)
	{
		this->~BufSendAlloc();

		const_cast<BufferOptions&>(bufferOptions) = bufSendAlloc.bufferOptions;
		sendDataPool = std::move(bufSendAlloc.sendDataPool);
		sendMsgPool = std::move(bufSendAlloc.sendMsgPool);
		memset(&bufSendAlloc, 0, sizeof(BufSendAlloc));
	}
	return *this;
}

char* BufSendAlloc::GetSendBuffer()
{
	return sendDataPool.alloc<char>() + sizeof(DWORD64);
}
MsgStreamWriter BufSendAlloc::CreateOutStream(short type, short msg)
{
	return{ GetSendBuffer(), bufferOptions.GetMaxDataSize(), type, msg };
}

WSABufSend BufSendAlloc::CreateBuff(DWORD nBytesDecomp, char* buffer, bool msg, CompressionType compType)
{
	DWORD nBytesComp = 0, nBytesSend = nBytesDecomp + sizeof(DWORD64);
	DWORD maxDataSize = bufferOptions.GetMaxDataSize();
	char* dest;

	if (msg)
	{
		char* temp = buffer;
		dest = buffer = sendMsgPool.alloc<char>();
		*(int*)(buffer + sizeof(DWORD64)) = *(int*)temp;
	}
	else
	{
		dest = buffer -= sizeof(DWORD64);
		if (nBytesDecomp > maxDataSize)
		{
			sendDataPool.destruct(buffer);
			return{};
		}
	}

	if (compType == BESTFIT)
	{
		if (nBytesDecomp >= bufferOptions.GetCompressionCO())
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}

	if (compType == SETCOMPRESSION)
	{
		DWORD temp = FileMisc::Compress((BYTE*)(buffer + maxDataSize + sizeof(DWORD64)), bufferOptions.GetMaxCompSize(), (const BYTE*)(buffer + sizeof(DWORD64)), nBytesDecomp, bufferOptions.GetCompression());
		if (nBytesComp < nBytesDecomp)
		{
			nBytesComp = temp;
			nBytesSend = nBytesComp;
			dest = buffer + maxDataSize;
		}
	}

	*(DWORD64*)(dest) = ((DWORD64)nBytesDecomp) << 32 | nBytesComp;

	WSABufSend buf;
	buf.Initialize(nBytesSend, dest, buffer);
	return buf;
}
void BufSendAlloc::FreeBuff(WSABufSend& buff)
{
	if (sendDataPool.InPool(buff.head))
		sendDataPool.dealloc(buff.head);
	else
		sendMsgPool.dealloc(buff.head);
}

const BufferOptions BufSendAlloc::GetBufferOptions() const
{
	return bufferOptions;
}