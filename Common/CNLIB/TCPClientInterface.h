#pragma once
#include "Socket.h"
#include <tchar.h>

class CAMSNETLIB TCPClientInterface
{
public:
	virtual bool Connect(const TCHAR* dest, const TCHAR* port, float timeOut = 5.0f) = 0;

	//Blocking
	virtual void Shutdown() = 0;

	//Non blocking
	virtual void Disconnect() = 0;

	//Should only be called once to intialy create receving thread
	//This MUST be called before you can senddata
	virtual bool RecvServData() = 0;

	virtual HANDLE SendServData(const char* data, DWORD nBytes) = 0;

	//send msg funtions used for requests, replies ect. they do not send data
	virtual void SendMsg(char type, char message) = 0;
	virtual void SendMsg(const std::tstring& user, char type, char message) = 0;

	//Should be called in function/msgHandler when ping msg is received from server
	virtual void Ping() = 0;

	virtual void SetFunction(cfunc function) = 0;

	virtual bool IsConnected() const = 0;
	virtual Socket& GetHost() = 0;

	//Returns obj you passed to ctor
	virtual void* GetObj() const = 0;
};

CAMSNETLIB TCPClientInterface* CreateClient(cfunc func, void(*const disconFunc)(), int compression = 9, void* obj = nullptr);
CAMSNETLIB void DestroyClient(TCPClientInterface*& client);
