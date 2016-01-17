#pragma once
#include <WinSock2.h>

struct WSABufExt : WSABUF
{
	WSABufExt()
		:
		curBytes(0),
		head(nullptr)
	{
		len = 0;
		buf = nullptr;
	}

	void Initalize(DWORD len, char* buf, char* head)
	{
		curBytes = 0;
		this->len = len;
		this->buf = buf;
		this->head = head;
	}
	
	DWORD curBytes;
	char* head;
};