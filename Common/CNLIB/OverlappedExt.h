#pragma once
#include "WSABufExt.h"

struct OverlappedExt : OVERLAPPED //dont wanna use virtual func because of mem pool and reuse of struct
{
	//int for alignment
	enum class OpType : int
	{
		recv,
		send,
		sendsingle,
		accept
	};

	OverlappedExt()
	{
		Reset();
	}
	OverlappedExt(OpType opType)
		:
		opType(opType)
	{
		Reset();
	}

	void Reset()
	{
		hEvent = NULL;
		//memset(this, 0, sizeof(OVERLAPPED)); //dont clear optype
	}

	OverlappedExt(OverlappedExt&& ol)
		:
		opType(ol.opType)
	{}

	OpType opType;
};

struct OverlappedSendInfo
{
	//single param needed for per client queue to know if refCount was incrimented
	OverlappedSendInfo(const WSABufSend& buff, int refCount)
		:
		sendBuff(buff),
		head(nullptr)
	{}


	//Returns true if need to dealloc sendBuff
	bool DecrementRefCount()
	{
		return InterlockedDecrement((LONG*)&refCount) == 0;
	}

	WSABufSend sendBuff;
	class OverlappedSend* head;
	int refCount;
};

struct OverlappedSend : OverlappedExt
{
	//single param needed for per client queue to know if refCount was incrimented
	OverlappedSend(OverlappedSendInfo* sendInfo)
		:
		OverlappedExt(OpType::send),
		sendInfo(sendInfo)
	{}

	void Initalize(OverlappedSendInfo* sendInfo)
	{
		Reset();
		this->opType = OpType::send;
		this->sendInfo = sendInfo;
	}

	OverlappedSendInfo* sendInfo;
};

struct OverlappedSendSingle : OverlappedExt
{
	OverlappedSendSingle(const WSABufSend& buff)
		:
		OverlappedExt(OpType::sendsingle),
		sendBuff(buff)
	{}

	WSABufSend sendBuff;
};

typedef OverlappedExt::OpType OpType;
