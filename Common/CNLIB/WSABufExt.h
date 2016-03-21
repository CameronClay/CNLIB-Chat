#pragma once
#include "MsgHeader.h"

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
		WSABufExt()
	{
		used = false;
	}

	WSABufRecv(const WSABufRecv& b)
	{
		*this = b;
	}

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
