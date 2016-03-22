#pragma once
#include "CNLIB/MsgHeader.h"
#include "CNLIB/BuffAllocator.h"

struct WSABufExt : public WSABUF
{
	WSABufExt()
		:
		head(nullptr)
	{
		len = 0;
		buf = nullptr;
	}

	void Initialize(DWORD len, char* buf, char* head)
	{
		this->len = len;
		this->buf = buf;
		this->head = head;
		curBytes = 0;
	}
	
	char* head;
	DWORD curBytes;
};

struct WSABufRecv : public WSABufExt
{
	WSABufRecv()
		:
		WSABufExt(),
		used(false)
	{}

	WSABufRecv& operator=(const WSABufRecv& b)
	{
		if (this != &b)
		{
			len = b.len;
			buf = b.buf;
			head = b.head;
			curBytes = b.curBytes;
			used = b.used;
		}
		return *this;
	}

	bool used;
};

struct WSABufSend : public WSABufExt
{
	WSABufSend()
		:
		WSABufExt(),
		alloc(nullptr)
	{}

	WSABufSend& operator=(const WSABufSend& b)
	{
		if (this != &b)
		{
			len = b.len;
			buf = b.buf;
			head = b.head;
			curBytes = b.curBytes;
			alloc = b.alloc;
		}
		return *this;
	}

	BuffAllocator* alloc;
};




