//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "TCPClientInterface.h"
#include "HeapAlloc.h"

class VerifyPing;
class TCPClient : public TCPClientInterface
{
public:
	//cfunc is a message handler, compression 1-9
	TCPClient(cfunc func, dcfunc disconFunc, int compression = 9, float serverDropTime = 60.0f, void* obj = nullptr);
	TCPClient(TCPClient&& client);
	~TCPClient();

	TCPClient& operator=(TCPClient&& client);


	bool Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, float timeOut = 5.0f);
	void Disconnect();
	void Shutdown();

	void Cleanup();

	HANDLE SendServData(const char* data, DWORD nBytes, CompressionType compType = BESTFIT);

	//Should only be called once to intialy create receving thread
	bool RecvServData();

	//send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(char type, char message);
	void SendMsg(const std::tstring& user, char type, char message);

	//Should be called in function/msgHandler when ping msg is received from server
	void Ping();

	void SetFunction(cfunc function);

	void RunDisconFunc();
	void SetShutdownReason(bool unexpected);

	void* GetObj() const;
	cfuncP GetFunction();
	Socket& GetHost();
	int GetCompression() const;
	CRITICAL_SECTION* GetSendSect();

	void SetServerDropTime(float time);
	float GetServerDropTime() const;

	VerifyPing* GetVerifyPing() const;

	dcfunc GetDisfunc() const;
	bool IsConnected() const;

private:
	Socket host; //server/host you are connected to
	cfunc function; //function/msgHandler
	dcfunc disconFunc; //function called when disconnect occurs
	void* obj; //passed to function/msgHandler for oop programming
	HANDLE recv; //thread to receving packets from server
	CRITICAL_SECTION sendSect; //used for synchonization
	const int compression; //compression client sends packets at
	bool unexpectedShutdown; //passed to disconnect handler
	float serverDropTime; //time before last packet was received that is treated as server losing connection
	VerifyPing* verifyPing; //used to detect if a server has lost connection
};