//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Socket.h"
#include <tchar.h>

class TCPClientInterface;
typedef void(*const dcfunc)(bool unexpected);
typedef void(*cfunc)(TCPClientInterface& client, const BYTE* data, DWORD nBytes, void* obj);
typedef void(**const cfuncP)(TCPClientInterface& client, const BYTE* data, DWORD nBytes, void* obj);

class CAMSNETLIB TCPClientInterface
{
public:
	virtual bool Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, float timeOut = 5.0f) = 0;

	virtual void Shutdown() = 0;
	virtual void Disconnect() = 0;

	virtual bool RecvServData() = 0;

	virtual HANDLE SendServData(const char* data, DWORD nBytes) = 0;

	virtual void SendMsg(char type, char message) = 0;
	virtual void SendMsg(const std::tstring& user, char type, char message) = 0;

	virtual void Ping() = 0;

	virtual void SetFunction(cfunc function) = 0;

	virtual bool IsConnected() const = 0;

	virtual Socket& GetHost() = 0;
	virtual void* GetObj() const = 0;
};

CAMSNETLIB TCPClientInterface* CreateClient(cfunc msgHandler, dcfunc disconFunc, int compression = 9, void* obj = nullptr);
CAMSNETLIB void DestroyClient(TCPClientInterface*& client);