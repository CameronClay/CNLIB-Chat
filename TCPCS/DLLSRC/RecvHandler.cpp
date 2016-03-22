#include "Stdafx.h"
#include "RecvHandler.h"
#include "CNLIB/MsgHeader.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/File.h"


//To do: have only one compressed buffer
RecvHandler::RecvHandler(const BufferOptions& buffOpts, UINT initialCap, RecvObserverI* observer)
	:
	recvBuffPool(buffOpts.GetMaxDatBuffSize(), initialCap + 1, buffOpts.GetPageSize()),  //only need maxDataSize, because compData buffer takes care of decompression
	decompData(recvBuffPool.alloc<char>()),
	ol(OpType::recv),
	primaryBuff(CreateBuffer(buffOpts)),
	secondaryBuff(CreateBuffer(buffOpts)),
	curBuff(&primaryBuff),
	nextBuff(&secondaryBuff),
	observer(observer)
{
	buffMap.reserve(initialCap);
}

RecvHandler::RecvHandler(RecvHandler&& recvHandler)
	:
	recvBuffPool(std::move(recvHandler.recvBuffPool)),
	decompData(recvHandler.decompData),
	buffMap(std::move(recvHandler.buffMap)),
	ol(recvHandler.ol),
	primaryBuff(recvHandler.primaryBuff),
	secondaryBuff(recvHandler.secondaryBuff),
	curBuff(recvHandler.curBuff),
	nextBuff(recvHandler.nextBuff),
	observer(recvHandler.observer)
{
	memset(&recvHandler, 0, sizeof(RecvHandler));
}

RecvHandler::~RecvHandler()
{
	//no need to free objects from pool because they will die when pool dies
}

RecvHandler& RecvHandler::operator=(RecvHandler&& recvHandler)
{
	if (this != &recvHandler)
	{
		this->~RecvHandler();

		recvBuffPool = std::move(recvHandler.recvBuffPool);
		decompData = recvHandler.decompData;
		buffMap = std::move(recvHandler.buffMap);
		ol = recvHandler.ol;
		primaryBuff = recvHandler.primaryBuff;
		secondaryBuff = recvHandler.secondaryBuff;
		curBuff = recvHandler.curBuff;
		nextBuff = recvHandler.nextBuff;
		observer = recvHandler.observer;

		memset(&recvHandler, 0, sizeof(RecvHandler));
	}

	return *this;
}

void RecvHandler::StartRead(Socket& pc)
{
	pc.ReadDataOl(curBuff, &ol);
}

bool RecvHandler::RecvDataCR(Socket& pc, DWORD bytesTrans, const BufferOptions& buffOpts, void* obj)
{
	//spin for a bit if buffer is still being used
	//or I guess could wrap entire thing in crit sect
	//guess a sleep would be acceptable because it would wake another iocp thread
	while (curBuff->used)
		Sleep(1);

	curBuff->used = true;

	char* ptr = curBuff->head;

	pc.ReadDataOl(nextBuff, &ol);

	while (bytesTrans)
	{
		DataHeader& header = *(DataHeader*)ptr;
		
		auto& it = buffMap.find(header.index); //Find if there is already a partial buffer with matching index
		if(!(ptr = RecvData(bytesTrans, ptr, it, header, buffOpts, obj)))
			return false;
	}

	std::swap(curBuff, nextBuff);

	curBuff->used = false;
	
	return true;
}

char* RecvHandler::RecvData(DWORD& bytesTrans, char* ptr, std::unordered_map<UINT, WSABufRecv>::iterator it, const DataHeader& header, const BufferOptions& buffOpts, void* obj)
{
	const DWORD bytesToRecv = ((header.size.up.nBytesComp) ? header.size.up.nBytesComp : header.size.up.nBytesDecomp);
	WSABufRecv& srcBuff = *curBuff;

	if (it != buffMap.end())
	{
		WSABufRecv& destBuff = it->second;
		bytesTrans = AppendBuffer(ptr, srcBuff, it->second, bytesToRecv, bytesTrans);

		if (destBuff.curBytes - sizeof(DataHeader) >= bytesToRecv)
		{
			const DWORD temp = bytesToRecv + sizeof(DataHeader);
			if (!Process(ptr, destBuff, bytesToRecv, header, buffOpts, obj))
				return nullptr;

			EraseFromMap(it);

			return ptr + bytesToRecv;
		}

		bytesTrans = 0;
		return ptr;
	}
	else
	{
		const DWORD initialBytesTrans = bytesTrans;
		//If there is a full data block ready for processing
		if (bytesTrans - sizeof(DataHeader) >= bytesToRecv)
		{
			bytesTrans -= bytesToRecv + sizeof(DataHeader);
			ptr += sizeof(DataHeader);
	
			if (!Process(ptr, srcBuff, bytesToRecv, header, buffOpts, obj))
				return nullptr;

			srcBuff.curBytes = 0;

			ptr += bytesToRecv;
			return ptr;
		}

		//Concatenate remaining data to front of buffer
		const DWORD bytesReceived = initialBytesTrans - bytesTrans;
		if (srcBuff.head != ptr)
			memcpy(srcBuff.head, ptr, bytesReceived);
		srcBuff.curBytes = bytesReceived;

		AppendToMap(header.index, srcBuff, buffOpts);
		//srcBuff.buf = srcBuff.head + bytesReceived;
		//ptr += bytesToRecv;

		return ptr;
	}
}


//char* RecvHandler::RecvData(DWORD& bytesTrans, char* ptr, DataHeader header, std::unordered_map<UINT, WSABufRecv>::iterator it, const BufferOptions& buffOpts, void* obj)
//{
//	const DWORD bytesToRecv = ((header.size.up.nBytesComp) ? header.size.up.nBytesComp : header.size.up.nBytesDecomp);
//	WSABufRecv& buff = *curBuff;
//	bytesTrans += buff.curBytes;
//	buff.curBytes = 0;
//
//	if (bytesTrans - sizeof(DataHeader) >= bytesToRecv)
//	{
//		const DWORD temp = bytesToRecv + sizeof(DataHeader);
//		//buff.curBytes += temp;
//		bytesTrans -= temp;
//		ptr += sizeof(DataHeader);
//	
//		if (!Process(ptr, buff, bytesToRecv, header, buffOpts, obj))
//			return nullptr;
//
//		//If appended partial block
//		if (it != buffMap.end())
//		{
//			EraseFromMap(it);
//		}
//
//		////If no partial blocks to copy to start of buffer
//		//else if (!bytesTrans)
//		//{
//		//	buff.buf = buff.head;
//		//	buff.curBytes = 0;
//		//}
//
//		return ptr + bytesToRecv;
//	}
//	//If the next block of data has not been fully received
//	else if (bytesToRecv <= buffOpts.GetMaxDataSize())
//	{
//		//Concatenate remaining data to buffer
//		const DWORD bytesReceived = bytesTrans - buff.curBytes;
//		//if (buff.head != ptr)
//		//	memcpy(buff.head, ptr, bytesReceived);
//		buff.curBytes = bytesReceived;
//		buff.buf = buff.head + bytesReceived;
//
//		if (it == buffMap.end())
//		{
//			AppendToMap(buff, header.index, buffOpts);
//		}
//
//		bytesTrans = 0;
//		return ptr;
//	}
//}

bool RecvHandler::Process(char* ptr, WSABufRecv& buff, DWORD bytesToRecv, const DataHeader& header, const BufferOptions& buffOpts, void* obj)
{
	//If data was compressed
	if (header.size.up.nBytesComp)
	{
		//Max Data size because it sends decomp if > than maxDataSize
		if (FileMisc::Decompress((BYTE*)decompData, buffOpts.GetMaxDataSize(), (const BYTE*)ptr, bytesToRecv) != UINT_MAX)	// Decompress data
			observer->OnNotify(decompData, header.size.up.nBytesDecomp, obj);
		else
			return nullptr;  //return value of false is an unrecoverable error
	}
	//If data was not compressed
	else
	{
		observer->OnNotify(ptr, header.size.up.nBytesDecomp, obj);
	}
}

DWORD RecvHandler::AppendBuffer(char* ptr, WSABufRecv& srcBuff, WSABufRecv& destBuff, DWORD bytesToRecv, DWORD bytesTrans)
{
	DWORD temp = min(bytesToRecv - destBuff.curBytes, bytesTrans);
	memcpy(destBuff.buf, ptr, temp);
	destBuff.curBytes += temp;

	return bytesTrans - temp;
}

void RecvHandler::EraseFromMap(std::unordered_map<UINT, WSABufRecv>::iterator it)
{
	buffMap.erase(it);
	FreeBuffer(it->second);
}
void RecvHandler::AppendToMap(UINT index, WSABufRecv& buff, const BufferOptions& buffOpts)
{
	buffMap.emplace(std::make_pair(index, buff));
	*curBuff = CreateBuffer(buffOpts);
}

WSABufRecv RecvHandler::CreateBuffer(const BufferOptions& buffOpts)
{
	char* temp = recvBuffPool.alloc<char>();
	WSABufRecv buff;
	buff.Initialize(buffOpts.GetMaxDataSize() + sizeof(MsgHeader), temp, temp);
	return buff;
}

void RecvHandler::FreeBuffer(WSABufRecv& buff)
{
	recvBuffPool.dealloc(buff.head);
}