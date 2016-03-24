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

	pc.ReadDataOl(nextBuff, &ol);

	char* ptr = curBuff->head;

	do
	{
		DataHeader& header = *(DataHeader*)ptr;
		
		auto& it = buffMap.find(header.index); //Find if there is already a partial buffer with matching index
		if (!(ptr = RecvData(bytesTrans, ptr, it, header, buffOpts, obj)))
		{
			curBuff->used = false;
			return false;
		}
	} while (bytesTrans);

	std::swap(curBuff, nextBuff);

	curBuff->used = false;
	
	return true;
}

char* RecvHandler::RecvData(DWORD& bytesTrans, char* ptr, std::unordered_map<short, WSABufRecv>::iterator it, const DataHeader& header, const BufferOptions& buffOpts, void* obj)
{
	const DWORD bytesToRecv = ((header.size.up.nBytesComp) ? header.size.up.nBytesComp : header.size.up.nBytesDecomp);
	WSABufRecv& srcBuff = *curBuff;

	if (it != buffMap.end())
	{
		WSABufRecv& destBuff = it->second;
		bytesTrans = AppendBuffer(ptr, srcBuff, it->second, bytesToRecv, bytesTrans);

		if (destBuff.curBytes - sizeof(DataHeader) >= bytesToRecv)
		{
			char* newPtr = Process(ptr, destBuff, destBuff.curBytes, header, buffOpts, obj);

			EraseFromMap(it);
			srcBuff.curBytes = 0;

			return newPtr;
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

			srcBuff.curBytes = 0;
	
			return Process(ptr, srcBuff, bytesToRecv, header, buffOpts, obj);
		}

		const DWORD bytesReceived = srcBuff.curBytes = initialBytesTrans - bytesTrans;
		if (bytesToRecv <= buffOpts.GetMaxDatBuffSize())
		{
			//Concatenate remaining data to front of buffer
			if (srcBuff.head != ptr)
				memcpy(srcBuff.head, ptr, bytesReceived);

			AppendToMap(header.index, srcBuff, true, buffOpts);
			//srcBuff.buf = srcBuff.head + bytesReceived;
			//ptr += bytesToRecv;
		}
		else
		{
			WSABufRecv buff;
			char* buffer = alloc<char>(bytesToRecv);
			memcpy(buffer, ptr, bytesReceived);
			buff.Initialize(bytesToRecv, buffer, buffer);

			AppendToMap(header.index, buff, false, buffOpts);
		}

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

char* RecvHandler::Process(char* ptr, WSABufRecv& buff, DWORD bytesToRecv, const DataHeader& header, const BufferOptions& buffOpts, void* obj)
{
	//If data was compressed
	if (header.size.up.nBytesComp)
	{
		//Max Data size because it sends decomp if > than maxDataSize
		if (FileMisc::Decompress((BYTE*)decompData, header.size.up.nBytesDecomp, (const BYTE*)ptr, bytesToRecv) != UINT_MAX)	// Decompress data
		{
			observer->OnNotify(decompData, header.size.up.nBytesDecomp, obj);
			return ptr + bytesToRecv;
		}

		return nullptr;  //return nullptr if compression failed
	}

	//If data was not compressed
	observer->OnNotify(ptr, header.size.up.nBytesDecomp, obj);
	return ptr + bytesToRecv;
}

DWORD RecvHandler::AppendBuffer(char* ptr, WSABufRecv& srcBuff, WSABufRecv& destBuff, DWORD bytesToRecv, DWORD bytesTrans)
{
	DWORD temp = min(bytesToRecv - destBuff.curBytes, bytesTrans);
	memcpy(destBuff.buf, ptr, temp);
	destBuff.curBytes += temp;

	return bytesTrans - temp;
}

void RecvHandler::EraseFromMap(std::unordered_map<short, WSABufRecv>::iterator it)
{
	buffMap.erase(it);
	FreeBuffer(it->second);
}
void RecvHandler::AppendToMap(short index, WSABufRecv& buff, bool newBuff, const BufferOptions& buffOpts)
{
	buffMap.emplace(index, buff);

	if (newBuff)
		*curBuff = CreateBuffer(buffOpts);
}

WSABufRecv RecvHandler::CreateBuffer(const BufferOptions& buffOpts)
{
	char* temp = recvBuffPool.alloc<char>();
	WSABufRecv buff;
	buff.Initialize(buffOpts.GetMaxDatBuffSize(), temp, temp);
	return buff;
}

void RecvHandler::FreeBuffer(WSABufRecv& buff)
{
	if (recvBuffPool.InPool(buff.head))
		recvBuffPool.dealloc(buff.head);
	else
		dealloc(buff.head);
}