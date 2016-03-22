#include "StdAfx.h"
#include "BufSendAlloc.h"
#include "CNLIB/File.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/MsgHeader.h"

DataPoolAllocator::DataPoolAllocator(const BufferOptions& bufferOptions, UINT maxDataSize, UINT sendBuffCount)
	:
	sendDataPool(maxDataSize + sizeof(MsgHeader), sendBuffCount, bufferOptions.GetPageSize())
{}
DataPoolAllocator::DataPoolAllocator(DataPoolAllocator&& dataPoolObs)
	:
	sendDataPool(std::move(dataPoolObs.sendDataPool))
{
	memset(&dataPoolObs, 0, sizeof(DataPoolAllocator));
}
DataPoolAllocator& DataPoolAllocator::operator=(DataPoolAllocator&& dataPoolObs)
{
	if (this != &dataPoolObs)
	{
		sendDataPool = std::move(dataPoolObs.sendDataPool);
		memset(&dataPoolObs, 0, sizeof(DataPoolAllocator));
	}
	return *this;
}

void DataPoolAllocator::dealloc(char* data, DWORD nBytes)
{
	sendDataPool.dealloc(data);
}
char* DataPoolAllocator::alloc(DWORD)
{
	return sendDataPool.alloc<char>();
}


DataCompPoolAllocator::DataCompPoolAllocator(const BufferOptions& bufferOptions, UINT maxDataSize, UINT sendCompBuffCount)
	:
	sendDataCompPool(maxDataSize + bufferOptions.GetMaxCompSize() + sizeof(DataHeader) * 2 + MSG_OFFSET, sendCompBuffCount, bufferOptions.GetPageSize())
{}
DataCompPoolAllocator::DataCompPoolAllocator(DataCompPoolAllocator&& dataPoolObs)
	:
	sendDataCompPool(std::move(dataPoolObs.sendDataCompPool))
{
	memset(&dataPoolObs, 0, sizeof(DataCompPoolAllocator));
}
DataCompPoolAllocator& DataCompPoolAllocator::operator=(DataCompPoolAllocator&& dataPoolObs)
{
	if (this != &dataPoolObs)
	{
		sendDataCompPool = std::move(dataPoolObs.sendDataCompPool);
		memset(&dataPoolObs, 0, sizeof(DataCompPoolAllocator));
	}
	return *this;
}

void DataCompPoolAllocator::dealloc(char* data, DWORD nBytes)
{
	sendDataCompPool.dealloc(data);
}
char* DataCompPoolAllocator::alloc(DWORD)
{
	return sendDataCompPool.alloc<char>();
}


MsgPoolAllocator::MsgPoolAllocator(UINT sendMsgBuffCount)
	:
	sendMsgPool(sizeof(MsgHeader), sendMsgBuffCount)
{}
MsgPoolAllocator::MsgPoolAllocator(MsgPoolAllocator&& msgPoolObs)
	:
	sendMsgPool(std::move(msgPoolObs.sendMsgPool))
{
	memset(&msgPoolObs, 0, sizeof(MsgPoolAllocator));
}
MsgPoolAllocator& MsgPoolAllocator::operator=(MsgPoolAllocator&& msgPoolObs)
{
	if (this != &msgPoolObs)
	{
		sendMsgPool = std::move(msgPoolObs.sendMsgPool);
		memset(&msgPoolObs, 0, sizeof(MsgPoolAllocator));
	}

	return *this;
}

void MsgPoolAllocator::dealloc(char* data, DWORD nBytes)
{
	sendMsgPool.dealloc(data);
}
char* MsgPoolAllocator::alloc(DWORD)
{
	return sendMsgPool.alloc<char>();
}


BufSendAlloc::BufSendAlloc(UINT maxDataSize, UINT sendBuffCount, UINT sendCompBuffCount, UINT sendMsgBuffCount, int compression, int compressionCO)
	:
	bufferOptions(maxDataSize, compression, compressionCO),
	dataPool(bufferOptions, maxDataSize, sendBuffCount),
	dataCompPool(bufferOptions, maxDataSize, sendCompBuffCount),
	msgPool(sendMsgBuffCount)
{}
BufSendAlloc::BufSendAlloc(BufSendAlloc&& bufSendAlloc)
	:
	bufferOptions(bufSendAlloc.bufferOptions),
	dataPool(std::move(bufSendAlloc.dataPool)),
	dataCompPool(std::move(bufSendAlloc.dataCompPool)),
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
		dataCompPool = std::move(bufSendAlloc.dataCompPool);
		msgPool = std::move(bufSendAlloc.msgPool);
		memset(&bufSendAlloc, 0, sizeof(BufSendAlloc));
	}
	return *this;
}

BuffSendInfo BufSendAlloc::GetSendBuffer(DWORD hiByteEstimate, CompressionType compType)
{
	return{ dataPool.alloc(NULL) + sizeof(DataHeader), compType, hiByteEstimate, bufferOptions.GetCompressionCO() };
}
BuffSendInfo BufSendAlloc::GetSendBuffer(BuffAllocator* alloc, DWORD nBytes, CompressionType compType)
{
	return{ alloc->alloc(nBytes + sizeof(DataHeader)) + sizeof(DataHeader), compType, nBytes, bufferOptions.GetCompressionCO() };
}

MsgStreamWriter BufSendAlloc::CreateOutStream(DWORD hiByteEstimate, short type, short msg, CompressionType compType)
{
	return{ GetSendBuffer(hiByteEstimate + sizeof(DataHeader)), bufferOptions.GetMaxDataSize(), type, msg };
}
MsgStreamWriter BufSendAlloc::CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg, CompressionType compType)
{
	return{ GetSendBuffer(alloc, nBytes), nBytes + sizeof(DataHeader), type, msg };
}

WSABufSend BufSendAlloc::CreateBuff(const BuffSendInfo& buffSendInfo, DWORD nBytesDecomp, bool msg, USHORT index, BuffAllocator* alloc)
{
	DWORD nBytesComp = 0, nBytesSend = nBytesDecomp + sizeof(DataHeader);
	DWORD maxDataSize = bufferOptions.GetMaxDataSize();
	WSABufSend buf;
	char* buffer = buffSendInfo.buffer;
	char* dest;

	if (msg)
	{
		char* temp = buffSendInfo.buffer;
		dest = buffer = msgPool.alloc();
		*(int*)(buffer + sizeof(DataHeader)) = *(int*)temp;

		buf.alloc = &msgPool;
	}
	else
	{
		dest = buffer -= sizeof(DataHeader);
		if (!alloc)
		{
			if (buffSendInfo.compType == SETCOMPRESSION)
				buf.alloc = &dataCompPool;
			else
				buf.alloc = &dataPool;

			if (nBytesDecomp > maxDataSize)
			{
				buf.alloc->dealloc(buffer);
				return{};
			}
		}
		else
		{
			buf.alloc = alloc;
		}
	}

	if (buffSendInfo.compType == SETCOMPRESSION)
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
