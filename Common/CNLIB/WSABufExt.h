#pragma once

struct WSABufSend : WSABUF
{
	WSABufSend()
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
	}
	
	char* head;
};

struct WSABufRecv : WSABufSend
{
	WSABufRecv()
		:
		WSABufSend()
	{}

	void Initialize(DWORD len, char* buf, char* head)
	{
		curBytes = 0;
		__super::Initialize(len, buf, head);
	}

	DWORD curBytes;
};

