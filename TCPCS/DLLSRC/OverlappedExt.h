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
	//single param needed for per client queue to know if refCount was incremented
	OverlappedSendInfo(const WSABufSend& buff, int refCount)
		:
		sendBuff(buff),
		head(nullptr),
		refCount(refCount)
	{}


	//Returns true if need to dealloc sendBuff
	bool DecrementRefCount()
	{
		return --refCount == 0;
	}

	WSABufSend sendBuff;
	struct OverlappedSend* head;
	std::atomic<int> refCount;
};

struct OverlappedSend : OverlappedExt
{
	//single param needed for per client queue to know if refCount was incremented
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

//struct OverlappedRecv: OverlappedExt
//{
//	//single param needed for per client queue to know if refCount was incremented
//	OverlappedRecv()
//		:
//		OverlappedExt(OpType::recv),
//		buf()
//	{}
//
//	WSABufExt* buf;
//};

typedef OverlappedExt::OpType OpType;
