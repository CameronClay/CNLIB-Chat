#pragma once
#include "Socket.h"
#include <tchar.h>

class TCPClient
{
public:
	TCPClient(cfunc func, void(*const disconFunc)(), void* obj, int compression);
	TCPClient(TCPClient&& client);
	~TCPClient();

	
	void Connect(const TCHAR* dest, const TCHAR* port, float timeOut);
	void Disconnect();

	HANDLE SendServData(const char* data, DWORD nBytes);
	void RecvServData();

	void SendMsg(std::tstring& user, char type, char message);

	static void WaitAndCloseHandle(HANDLE& hnd);

	void Ping();
	void SetFunction(cfunc function);

	void* GetObj() const;
	cfuncP GetFunction();
	Socket& GetHost();
	void WaitForRecvThread() const;
	int GetCompression() const;

	void(*GetDisfunc()) ();
	bool Connected() const;

private:
	Socket host;
	cfunc function;
	void(*const disconFunc)();
	void* obj;
	HANDLE recv;
	int compression;
};
