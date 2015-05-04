#pragma once
#include "Socket.h"
#include <tchar.h>

class TCPClient
{
public:
	//cfunc is a message handler, compression 1-9
	TCPClient(cfunc func, void(*const disconFunc)(), void* obj, int compression);
	TCPClient(TCPClient&& client);
	~TCPClient();

	TCPClient& operator=(TCPClient&& client);


	void Connect(const TCHAR* dest, const TCHAR* port, float timeOut);
	void Disconnect();

	HANDLE SendServData(const char* data, DWORD nBytes);
	void RecvServData();

	//send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(char type, char message);
	void SendMsg(std::tstring& user, char type, char message);

	static void WaitAndCloseHandle(HANDLE& hnd);

	void Ping();
	void SetFunction(cfunc function);

	void* GetObj() const;
	cfuncP GetFunction();
	Socket& GetHost();
	void WaitForRecvThread() const;
	int GetCompression() const;
	CRITICAL_SECTION* GetSendSect();

	void(*GetDisfunc()) ();
	bool Connected() const;

private:
	Socket host;
	cfunc function;
	void(*const disconFunc)();
	void* obj;
	HANDLE recv;
	CRITICAL_SECTION sendSect;
	const int compression;
};
