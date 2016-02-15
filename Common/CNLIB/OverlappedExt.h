#pragma once
#include "WSABufExt.h"

struct OverlappedExt : OVERLAPPED //dont wanna use virtual func because of mem pool and reuse of struct
{
	OverlappedExt()
	{
		memset(this, 0, sizeof(OverlappedExt));
	}

	OverlappedExt(OverlappedExt&& ol)
		:
		opType(ol.opType)
	{}

	//int for alignment
	enum class OpType : int
	{
		recv,
		send,
		sendmsg,
		accept
	} opType;

	OverlappedExt(OpType opType)
		:
		opType(opType)
	{
		Reset();
	}

	void Reset()
	{
		memset(this, 0, sizeof(OVERLAPPED)); //dont clear optype
	}
};

struct OverlappedSendInfo
{
	//single param needed for per client queue to know if refCount was incrimented
	OverlappedSendInfo(const WSABufSend& buff, bool single, int refCount)
		:
		sendBuff(buff),
		head(nullptr),
		refCount(single ? -1 : refCount)
	{}

	inline bool IsSingle()
	{
		return refCount == -1;
	}

	//Returns true if need to dealloc sendBuff
	bool DecrementRefCount()
	{
		return IsSingle() ? true : (InterlockedDecrement((LONG*)&refCount) == 0);
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
		sendInfo(sendInfo)
	{}

	void Initalize(OverlappedSendInfo* sendInfo)
	{
		opType = OpType::send;
		this->sendInfo = sendInfo;
	}

	OverlappedSendInfo* sendInfo;
};

typedef OverlappedExt::OpType OpType;
