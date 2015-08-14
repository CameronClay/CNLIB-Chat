#pragma once
#include <stdlib.h>
#include <vector>
#include "Socket.h"
#include "Ping.h"

class CAMSNETLIB TCPServInterface
{
public:
	struct CAMSNETLIB ClientData
	{
		ClientData(class TCPServ& serv, Socket pc, sfunc func, USHORT recvIndex);
		ClientData(ClientData&& clint);
		ClientData& operator=(ClientData&& data);

		Socket pc;
		sfunc func;
		PingHandler pingHandler;
		std::tstring user;

		//DO NOT TOUCH
		USHORT recvIndex;
		HANDLE recvThread;

		//User defined object to store extra data in the struct
		void* obj;
	};

	virtual bool AllowConnections(const LIB_TCHAR* port) = 0;


	virtual HANDLE SendClientData(const char* data, DWORD nBytes, Socket addr, bool single) = 0;
	virtual HANDLE SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs) = 0;
	virtual HANDLE SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs) = 0;

	virtual void SendMsg(Socket pc, bool single, char type, char message) = 0;
	virtual void SendMsg(Socket* pcs, USHORT nPcs, char type, char message) = 0;
	virtual void SendMsg(std::vector<Socket>& pcs, char type, char message) = 0;
	virtual void SendMsg(const std::tstring& user, char type, char message) = 0;

	virtual ClientData* FindClient(const std::tstring& user) const = 0;

	virtual void Shutdown() = 0;

	virtual ClientData** GetClients() const = 0;
	virtual USHORT ClientCount() const = 0;
	virtual void SetPingInterval(float interval) = 0;

	virtual bool MaxClients() const = 0;
	virtual bool IsConnected() const = 0;

	virtual Socket& GetHost() = 0;

	virtual void* GetObj() const = 0;
};

typedef TCPServInterface::ClientData ClientData;
typedef void(*const customFunc)(ClientData* data);


CAMSNETLIB TCPServInterface* CreateServer(sfunc func, customFunc conFunc, customFunc disFunc, USHORT maxCon = 20, int compression = 9, float pingInterval = 30.0f, void* obj = nullptr);
CAMSNETLIB void DestroyServer(TCPServInterface*& server);
