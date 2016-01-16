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

	void Initalize(DWORD len, char* buf)
	{
		curBytes = 0;
		this->len = len;
		this->buf = head = buf;
	}
	
	DWORD curBytes;
	char* head;
};