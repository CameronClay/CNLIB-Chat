#pragma once
#include <Windows.h>
#include "SendBuffer.h"

struct OverlappedExt : OVERLAPPED //dont wanna use virtual func because of mem pool and reuse of struct
{
	OverlappedExt()
	{
		memset(this, 0, sizeof(OverlappedExt));
	}

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

struct OverlappedSend : OverlappedExt
{
	OverlappedSend(USHORT refCount)
		:
		sendBuff({0, nullptr}),
		refCount(refCount),
		head(nullptr)
	{}

	void InitInstance(OpType opType, const WSABUF& buf, OverlappedExt* hd)
	{
		sendBuff = buf;
		head = hd;
	}

	//Returns true if need to dealloc sendBuff
	bool DecrementRefCount()
	{
		return --refCount == 0;
	}

	WSABUF sendBuff;
	OverlappedExt* head;
	USHORT refCount;
};

typedef OverlappedExt::OpType OpType;