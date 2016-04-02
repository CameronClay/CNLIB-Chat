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
	primaryBuff(CreateBuffer(buffOpts, false)),
	secondaryBuff(CreateBuffer(buffOpts, false)),
	curBuff(&primaryBuff),
	nextBuff(&secondaryBuff),
	savedBuff(),
	observer(observer)
{}
RecvHandler::RecvHandler(RecvHandler&& recvHandler)
	:
	recvBuffPool(std::move(recvHandler.recvBuffPool)),
	decompData(recvHandler.decompData),
	ol(recvHandler.ol),
	primaryBuff(recvHandler.primaryBuff),
	secondaryBuff(recvHandler.secondaryBuff),
	curBuff(recvHandler.curBuff),
	nextBuff(recvHandler.nextBuff),
	savedBuff(recvHandler.savedBuff),
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
		if (!(ptr = RecvData(bytesTrans, ptr, buffOpts, obj)))
		{
			curBuff->used = false;
			return false;
		}
	} while (bytesTrans);

	std::swap(curBuff, nextBuff);

	//nextBuff because of swap
	nextBuff->used = false;
	
	return true;
}

char* RecvHandler::RecvData(DWORD& bytesTrans, char* ptr, const BufferOptions& buffOpts, void* obj)
{
	WSABufRecv& srcBuff = *curBuff;

	if (savedBuff.curBytes)
	{
		if (savedBuff.curBytes < sizeof(DataHeader))
		{
			const DWORD temp = min(sizeof(DataHeader) - savedBuff.curBytes, bytesTrans);
			memcpy(savedBuff.head + savedBuff.curBytes, ptr, temp);

			bytesTrans -= temp;
			ptr += temp;
			savedBuff.curBytes += temp;

			if (savedBuff.curBytes < sizeof(DataHeader))
			{
				bytesTrans = 0;
				return ptr;
			}
		}

		DataHeader& header = *(DataHeader*)savedBuff.head;
		const DWORD bytesToRecv = ((header.size.up.nBytesComp) ? header.size.up.nBytesComp : header.size.up.nBytesDecomp);
		if ((bytesToRecv == MAXDWORD) || (bytesToRecv == 0)) //unrecoverable error
			return nullptr;

		DWORD amountAppended = 0;
		bytesTrans = AppendBuffer(ptr, savedBuff, amountAppended, bytesToRecv, bytesTrans);
		ptr += amountAppended;

		if (savedBuff.curBytes - sizeof(DataHeader) >= bytesToRecv)
		{
			Process(savedBuff.head + sizeof(DataHeader), bytesToRecv, header, buffOpts, obj);

			FreeBuffer(savedBuff);
			srcBuff.curBytes = 0;
		}

		return ptr;
	}
	else
	{
		if (bytesTrans < sizeof(DataHeader))
		{
			if (srcBuff.head != ptr)
				memcpy(srcBuff.head, ptr, bytesTrans);

			srcBuff.curBytes = bytesTrans;
			SaveBuff(srcBuff, true, buffOpts);

			bytesTrans = 0;
			return ptr;
		}

		DataHeader& header = *(DataHeader*)ptr;
		const DWORD bytesToRecv = ((header.size.up.nBytesComp) ? header.size.up.nBytesComp : header.size.up.nBytesDecomp);
		if ((bytesToRecv == MAXDWORD) || (bytesToRecv == 0)) //unrecoverable error
			return nullptr;

		//If there is a full data block ready for processing
		if (bytesTrans - sizeof(DataHeader) >= bytesToRecv)
		{
			bytesTrans -= bytesToRecv + sizeof(DataHeader);
			ptr += sizeof(DataHeader);

			srcBuff.curBytes = 0;

			return Process(ptr, bytesToRecv, header, buffOpts, obj);
		}

		if (bytesToRecv <= buffOpts.GetMaxDatBuffSize())
		{
			//Concatenate remaining data to front of buffer
			if (srcBuff.head != ptr)
				memcpy(srcBuff.head, ptr, bytesTrans);

			srcBuff.curBytes = bytesTrans;
			SaveBuff(srcBuff, true, buffOpts);
		}
		else
		{
			char* buffer = alloc<char>(bytesToRecv + sizeof(DataHeader));
			memcpy(buffer, ptr, bytesTrans);

			WSABufRecv buff;
			buff.Initialize(bytesToRecv, buffer, buffer, bytesTrans);

			SaveBuff(buff, false, buffOpts);
		}

		bytesTrans = 0;
		return ptr;
	}
}

void RecvHandler::Reset()
{
	curBuff->curBytes = 0;
	curBuff->used = false;

	nextBuff->curBytes = 0;
	nextBuff->used = false;

	if (savedBuff.head)
		FreeBuffer(savedBuff);
}

char* RecvHandler::Process(char* ptr, DWORD bytesToRecv, const DataHeader& header, const BufferOptions& buffOpts, void* obj)
{
	//If data was compressed
	if (header.size.up.nBytesComp)
	{
		char* dest = (header.size.up.nBytesDecomp > buffOpts.GetMaxDataSize()) ? alloc<char>(header.size.up.nBytesDecomp) : decompData;

		//Max Data size because it sends decomp if > than maxDataSize
		if (FileMisc::Decompress((BYTE*)dest, header.size.up.nBytesDecomp, (const BYTE*)ptr, bytesToRecv) != UINT_MAX)	// Decompress data
		{
			observer->OnNotify(dest, header.size.up.nBytesDecomp, obj);
			if (dest != decompData)
				dealloc(dest);

			return ptr + bytesToRecv;
		}

		return nullptr;  //return nullptr if compression failed
	}

	if (*(short*)ptr != -1)
	{
		int a = 0;
	}

	//If data was not compressed
	observer->OnNotify(ptr, header.size.up.nBytesDecomp, obj);
	return ptr + bytesToRecv;
}
DWORD RecvHandler::AppendBuffer(char* ptr, WSABufRecv& destBuff, DWORD& amountAppended, DWORD bytesToRecv, DWORD bytesTrans)
{
	const DWORD temp = amountAppended = min(bytesToRecv + sizeof(DataHeader) - destBuff.curBytes, bytesTrans);
	memcpy(destBuff.head + destBuff.curBytes, ptr, temp);
	destBuff.curBytes += temp;

	return bytesTrans - temp;
}

void RecvHandler::SaveBuff(const WSABufRecv& buff, bool newBuff, const BufferOptions& buffOpts)
{
	savedBuff = buff;

	if (newBuff)
		*curBuff = CreateBuffer(buffOpts, true);
}

WSABufRecv RecvHandler::CreateBuffer(const BufferOptions& buffOpts, bool used)
{
	char* temp = recvBuffPool.alloc<char>();
	WSABufRecv buff;
	buff.Initialize(buffOpts.GetMaxDatBuffSize(), temp, temp);
	buff.used = used;
	return buff;
}
void RecvHandler::FreeBuffer(WSABufRecv& buff)
{
	if (recvBuffPool.InPool(buff.head))
		recvBuffPool.dealloc(buff.head);
	else
		dealloc(buff.head);

	savedBuff.curBytes = 0;
}