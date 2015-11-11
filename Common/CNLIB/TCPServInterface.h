//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include <stdlib.h>
#include <vector>
#include "Socket.h"
#include "Ping.h"
#include "CompressionTypes.h"


class CAMSNETLIB TCPServInterface
{
public:
	struct ClientData;
	typedef void(*sfunc)(TCPServInterface& serv, ClientData* const client, const BYTE* data, DWORD nBytes, void* obj);
	typedef void(**const sfuncP)(TCPServInterface& serv, ClientData* const client, const BYTE* data, DWORD nBytes, void* obj);

	struct CAMSNETLIB ClientData
	{
		ClientData(class TCPServ& serv, Socket pc, sfunc func, USHORT recvIndex);
		ClientData(ClientData&& clint);
		ClientData& operator=(ClientData&& data);

		Socket pc;
		sfunc func;
		std::tstring user;

		//DO NOT TOUCH
		USHORT recvIndex;
		HANDLE recvThread;

		//User defined object to store extra data in the struct
		void* obj;
	};

	virtual bool AllowConnections(const LIB_TCHAR* port) = 0;


	virtual void SendClientData(const char* data, DWORD nBytes, Socket addr, bool single, CompressionType compType = BESTFIT) = 0;
	virtual void SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, CompressionType compType = BESTFIT) = 0;
	virtual void SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType = BESTFIT) = 0;

	virtual HANDLE SendClientDataThread(const char* data, DWORD nBytes, Socket addr, bool single, CompressionType compType = BESTFIT) = 0;
	virtual HANDLE SendClientDataThread(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, CompressionType compType = BESTFIT) = 0;
	virtual HANDLE SendClientDataThread(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType = BESTFIT) = 0;

	virtual void SendMsg(Socket pc, bool single, char type, char message) = 0;
	virtual void SendMsg(Socket* pcs, USHORT nPcs, char type, char message) = 0;
	virtual void SendMsg(std::vector<Socket>& pcs, char type, char message) = 0;
	virtual void SendMsg(const std::tstring& user, char type, char message) = 0;

	virtual ClientData* FindClient(const std::tstring& user) const = 0;
	virtual void DisconnectClient(ClientData* client) = 0;

	virtual void Shutdown() = 0;

	virtual ClientData** GetClients() const = 0;
	virtual USHORT ClientCount() const = 0;

	virtual void SetPingInterval(float interval) = 0;
	virtual float GetPingInterval() const = 0;

	virtual bool MaxClients() const = 0;
	virtual bool IsConnected() const = 0;

	virtual Socket& GetHost() = 0;

	virtual void* GetObj() const = 0;
};

typedef TCPServInterface::ClientData ClientData;
typedef void(*const customFunc)(ClientData* data);

typedef TCPServInterface::sfunc sfunc;
typedef TCPServInterface::sfuncP sfuncP;


CAMSNETLIB TCPServInterface* CreateServer(sfunc msgHandler, customFunc conFunc, customFunc disFunc, USHORT maxCon = 20, int compression = 9, float pingInterval = 30.0f, void* obj = nullptr);
CAMSNETLIB void DestroyServer(TCPServInterface*& server);
