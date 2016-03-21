#include "StdAfx.h"
#include "BufSendAlloc.h"
#include "CNLIB/File.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/MsgHeader.h"

BufSendAlloc::BufSendAlloc(UINT maxDataSize, UINT sendBuffCount, UINT sendMsgBuffCount, int compression, int compressionCO)
	:
	bufferOptions(maxDataSize, compression, compressionCO),
	sendDataPool(maxDataSize + bufferOptions.GetMaxCompSize() + sizeof(DataHeader) * 2 + MSG_OFFSET, sendBuffCount, bufferOptions.GetPageSize()), //extra DataHeader incase it sends compressed data, because data written to initial buffer is offset by sizeof(DataHeader)
	sendMsgPool(sizeof(MsgHeader), sendMsgBuffCount)
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
	return sendDataPool.alloc<char>() + sizeof(DataHeader);
}
MsgStreamWriter BufSendAlloc::CreateOutStream(short type, short msg)
{
	return{ GetSendBuffer(), bufferOptions.GetMaxDataSize(), type, msg };
}

WSABufExt BufSendAlloc::CreateBuff(DWORD nBytesDecomp, char* buffer, bool msg, USHORT index, CompressionType compType)
{
	DWORD nBytesComp = 0, nBytesSend = nBytesDecomp + sizeof(DataHeader);
	DWORD maxDataSize = bufferOptions.GetMaxDataSize();
	char* dest;

	if (msg)
	{
		char* temp = buffer;
		dest = buffer = sendMsgPool.alloc<char>();
		*(int*)(buffer + sizeof(DataHeader)) = *(int*)temp;
	}
	else
	{
		dest = buffer -= sizeof(DataHeader);
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
		DWORD temp = FileMisc::Compress((BYTE*)(buffer + maxDataSize + sizeof(DataHeader)), bufferOptions.GetMaxCompSize(), (const BYTE*)(buffer + sizeof(DataHeader)), nBytesDecomp, bufferOptions.GetCompression());
		if (nBytesComp < nBytesDecomp)
		{
			nBytesComp = temp;
			nBytesSend = nBytesComp;
			dest = buffer + maxDataSize;
		}
	}

	DataHeader& header = *(DataHeader*)dest;
	header.size.up.nBytesComp = nBytesComp;
	header.size.up.nBytesDecomp = nBytesDecomp;
	header.index = (index == -1) ? ++bufIndex : index;

	WSABufExt buf;
	buf.Initialize(nBytesSend, dest, buffer);
	return buf;
}
void BufSendAlloc::FreeBuff(WSABufExt& buff)
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
