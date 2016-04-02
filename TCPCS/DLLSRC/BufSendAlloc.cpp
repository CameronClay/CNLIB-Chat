#include "StdAfx.h"
#include "BufSendAlloc.h"
#include "CNLIB/File.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/MsgHeader.h"

DataPoolAllocator::DataPoolAllocator(const BufferOptions& bufferOptions, UINT sendBuffCount)
	:
	sendDataPool(bufferOptions.GetMaxDataBuffSize(), sendBuffCount, bufferOptions.GetPageSize())
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


DataCompPoolAllocator::DataCompPoolAllocator(const BufferOptions& bufferOptions, UINT sendCompBuffCount)
	:
	sendDataCompPool(bufferOptions.GetMaxDataBuffSize() + bufferOptions.GetMaxDataCompSize(), sendCompBuffCount, bufferOptions.GetPageSize())
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


BufSendAlloc::BufSendAlloc(UINT maxDataBuffSize, UINT sendBuffCount, UINT sendCompBuffCount, UINT sendMsgBuffCount, int compression, int compressionCO)
	:
	bufferOptions(maxDataBuffSize, compression, compressionCO),
	dataPool(bufferOptions, sendBuffCount),
	dataCompPool(bufferOptions, sendCompBuffCount),
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
BufSendAlloc::~BufSendAlloc()
{
	//no need to free objects from pool because they will die when pool dies
}

BuffSendInfo BufSendAlloc::GetSendBuffer(DWORD hiByteEstimate, CompressionType compType)
{
	BuffSendInfo si(compType, hiByteEstimate, bufferOptions.GetCompressionCO());
	hiByteEstimate += si.maxCompSize;
	si.alloc = (hiByteEstimate > bufferOptions.GetMaxDataSize()) ? (BuffAllocator*)&defAlloc : (si.maxCompSize ? (BuffAllocator*)&dataCompPool : (BuffAllocator*)&dataPool);
	si.buffer = si.maxCompSize ? si.alloc->alloc(hiByteEstimate + sizeof(DataHeader)) : (si.alloc->alloc(hiByteEstimate + sizeof(DataHeader)) + sizeof(DataHeader));

	return si;
}
BuffSendInfo BufSendAlloc::GetSendBuffer(BuffAllocator* alloc, DWORD nBytes, CompressionType compType)
{
	BuffSendInfo si(compType, nBytes, bufferOptions.GetCompressionCO());
	nBytes += si.maxCompSize;
	si.alloc = alloc;
	si.buffer = si.maxCompSize ? alloc->alloc(nBytes + sizeof(DataHeader)) : (alloc->alloc(nBytes + sizeof(DataHeader)) + sizeof(DataHeader));
	return si;
}

MsgStreamWriter BufSendAlloc::CreateOutStream(DWORD hiByteEstimate, short type, short msg, CompressionType compType)
{
	return{ GetSendBuffer(hiByteEstimate + MSG_OFFSET, compType), hiByteEstimate, type, msg };
}
MsgStreamWriter BufSendAlloc::CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg, CompressionType compType)
{
	return{ GetSendBuffer(alloc, nBytes + MSG_OFFSET, compType), nBytes, type, msg };
}


DWORD BufSendAlloc::CompressBuffer(char*& dest, DWORD& nBytesSend, DWORD destStart, DWORD maxCompSize, int compression)
{
	const DWORD temp = FileMisc::Compress((BYTE*)(dest + destStart + sizeof(DataHeader)), maxCompSize, (const BYTE*)dest, nBytesSend, compression);
	if (temp < nBytesSend)
	{
		dest += destStart;
		return nBytesSend = temp;
	}
	return 0;
}

WSABufSend BufSendAlloc::CreateBuff(const BuffSendInfo& buffSendInfo, DWORD nBytesDecomp, bool msg)
{
	const DWORD maxDataSize = bufferOptions.GetMaxDataSize();
	DWORD nBytesComp = 0, nBytesSend = nBytesDecomp;
	char *buffer = buffSendInfo.buffer, *dest = buffer;
	char* tempDest = dest;
	WSABufSend buf;
	buf.alloc = buffSendInfo.alloc;

	if (msg)
	{
		char* temp = buffSendInfo.buffer;
		dest = buffer = msgPool.alloc();
		memcpy(buffer + sizeof(DataHeader), temp, MSG_OFFSET);

		buf.alloc = &msgPool;
	}
	else
	{
		if ((buf.alloc == &dataPool) || (buf.alloc == &dataCompPool))
		{
			if (buffSendInfo.compType == SETCOMPRESSION)
				nBytesComp = CompressBuffer(dest, nBytesSend, maxDataSize, buffSendInfo.maxCompSize, bufferOptions.GetCompression());
			else
				dest = buffer -= sizeof(DataHeader);
		}
		else
		{
			if (buffSendInfo.compType == SETCOMPRESSION)
				nBytesComp = CompressBuffer(dest, nBytesSend, nBytesDecomp, buffSendInfo.maxCompSize, bufferOptions.GetCompression());
			else
				dest = buffer -= sizeof(DataHeader);
		}
	}

	DataHeader& header = *(DataHeader*)dest;
	header.size.up.nBytesComp = nBytesComp;
	header.size.up.nBytesDecomp = nBytesDecomp;

	buf.Initialize(nBytesSend + sizeof(DataHeader), dest, buffer);
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
