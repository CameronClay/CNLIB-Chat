#include "StdAfx.h"
#include "BufSendAlloc.h"
#include "CNLIB/File.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/MsgHeader.h"

DataPoolObserver::DataPoolObserver(const BufferOptions& bufferOptions, UINT maxDataSize, UINT sendBuffCount)
	:
	sendDataPool(maxDataSize + bufferOptions.GetMaxCompSize() + sizeof(DataHeader) * 2 + MSG_OFFSET, sendBuffCount, bufferOptions.GetPageSize())
{}

DataPoolObserver::DataPoolObserver(DataPoolObserver&& dataPoolObs)
	:
	sendDataPool(std::move(dataPoolObs.sendDataPool))
{
	memset(&dataPoolObs, 0, sizeof(DataPoolObserver));
}
DataPoolObserver& DataPoolObserver::operator=(DataPoolObserver&& dataPoolObs)
{
	if (this != &dataPoolObs)
	{
		sendDataPool = std::move(dataPoolObs.sendDataPool);
		memset(&dataPoolObs, 0, sizeof(DataPoolObserver));
	}
	return *this;
}

void DataPoolObserver::dealloc(char* data, DWORD nBytes)
{
	sendDataPool.dealloc(data);
}

char* DataPoolObserver::alloc(DWORD)
{
	return sendDataPool.alloc<char>();
}


MsgPoolObserver::MsgPoolObserver(UINT sendMsgBuffCount)
	:
	sendMsgPool(sizeof(MsgHeader), sendMsgBuffCount)
{}

MsgPoolObserver::MsgPoolObserver(MsgPoolObserver&& msgPoolObs)
	:
	sendMsgPool(std::move(msgPoolObs.sendMsgPool))
{
	memset(&msgPoolObs, 0, sizeof(MsgPoolObserver));
}
MsgPoolObserver& MsgPoolObserver::operator=(MsgPoolObserver&& msgPoolObs)
{
	if (this != &msgPoolObs)
	{
		sendMsgPool = std::move(msgPoolObs.sendMsgPool);
		memset(&msgPoolObs, 0, sizeof(MsgPoolObserver));
	}

	return *this;
}
void MsgPoolObserver::dealloc(char* data, DWORD nBytes)
{
	sendMsgPool.dealloc(data);
}

char* MsgPoolObserver::alloc(DWORD)
{
	return sendMsgPool.alloc<char>();
}


BufSendAlloc::BufSendAlloc(UINT maxDataSize, UINT sendBuffCount, UINT sendMsgBuffCount, int compression, int compressionCO)
	:
	bufferOptions(maxDataSize, compression, compressionCO),
	dataPool(bufferOptions, maxDataSize, sendBuffCount),
	msgPool(sendMsgBuffCount)
{}
BufSendAlloc::BufSendAlloc(BufSendAlloc&& bufSendAlloc)
	:
	bufferOptions(bufSendAlloc.bufferOptions),
	dataPool(std::move(bufSendAlloc.dataPool)),
	msgPool(std::move(bufSendAlloc.msgPool))
{
	memset(&bufSendAlloc, 0, sizeof(BufSendAlloc));
}
BufSendAlloc& BufSendAlloc::operator=(BufSendAlloc&& bufSendAlloc)
{
	if (this != &bufSendAlloc)
	{
		this->~BufSendAlloc();

		const_cast<BufferOptions&>(bufferOptions) = bufSendAlloc.bufferOptions;
		dataPool = std::move(bufSendAlloc.dataPool);
		msgPool = std::move(bufSendAlloc.msgPool);
		memset(&bufSendAlloc, 0, sizeof(BufSendAlloc));
	}
	return *this;
}

char* BufSendAlloc::GetSendBuffer()
{
	return dataPool.alloc(NULL) + sizeof(DataHeader);
}
char* BufSendAlloc::GetSendBuffer(BuffAllocator* alloc, DWORD nBytes)
{
	return alloc->alloc(nBytes + sizeof(DataHeader)) + sizeof(DataHeader);
}

MsgStreamWriter BufSendAlloc::CreateOutStream(short type, short msg)
{
	return{ GetSendBuffer(), bufferOptions.GetMaxDataSize(), type, msg };
}

MsgStreamWriter BufSendAlloc::CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg)
{
	return{ GetSendBuffer(alloc, nBytes), nBytes + sizeof(DataHeader), type, msg };
}

WSABufSend BufSendAlloc::CreateBuff(DWORD nBytesDecomp, char* buffer, bool msg, USHORT index, CompressionType compType, BuffAllocator* alloc)
{
	DWORD nBytesComp = 0, nBytesSend = nBytesDecomp + sizeof(DataHeader);
	DWORD maxDataSize = bufferOptions.GetMaxDataSize();
	WSABufSend buf;
	char* dest;

	if (msg)
	{
		char* temp = buffer;
		dest = buffer = msgPool.alloc();
		*(int*)(buffer + sizeof(DataHeader)) = *(int*)temp;

		buf.alloc = &msgPool;
	}
	else
	{
		dest = buffer -= sizeof(DataHeader);
		if (!alloc)
		{
			buf.alloc = &dataPool;

			if (nBytesDecomp > maxDataSize)
			{
				dataPool.dealloc(buffer);
				return{};
			}
		}
		else
		{
			buf.alloc = alloc;
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

	buf.Initialize(nBytesSend, dest, buffer);
	return buf;
}

void BufSendAlloc::FreeBuff(WSABufSend& buff)
{
	buff.alloc->dealloc(buff.head, buff.len);
}

const BufferOptions BufSendAlloc::GetBufferOptions() const
{
	return bufferOptions;
}
