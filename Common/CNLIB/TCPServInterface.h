//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include <stdlib.h>
#include <vector>
#include "Socket.h"
#include "SocketListen.h"
#include "Ping.h"
#include "CompressionTypes.h"
#include "IPv.h"


class CAMSNETLIB TCPServInterface : public PingHI
{
public:
	struct ClientData;
	typedef void(*sfunc)(TCPServInterface& serv, ClientData* const client, const BYTE* data, DWORD nBytes);
	typedef void(**const sfuncP)(TCPServInterface& serv, ClientData* const client, const BYTE* data, DWORD nBytes);

	struct CAMSNETLIB ClientData
	{
		ClientData(TCPServInterface& serv, Socket pc, sfunc func);
		ClientData(ClientData&& clint);
		ClientData& operator=(ClientData&& data);

		Socket pc;
		sfunc func;
		std::tstring user;

		//User defined object to store extra data in the struct
		void* obj;
	};

	class CAMSNETLIB ClientAccess
	{
	public:
		ClientAccess(ClientData** clients);
		ClientData* operator+(USHORT amount);
		ClientData* operator-(USHORT amount);
		ClientData* operator[](USHORT index);
	private:
		const ClientData** clients;
	};

	virtual IPv AllowConnections(const LIB_TCHAR* port, IPv ipv = ipboth) = 0;

	virtual char* GetSendBuffer() = 0;

	virtual void SendClientData(const char* data, DWORD nBytes, Socket addr, bool single, CompressionType compType = BESTFIT) = 0;
	virtual void SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, CompressionType compType = BESTFIT) = 0;
	virtual void SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType = BESTFIT) = 0;

	virtual void SendMsg(Socket pc, bool single, char type, char message) = 0;
	virtual void SendMsg(Socket* pcs, USHORT nPcs, char type, char message) = 0;
	virtual void SendMsg(std::vector<Socket>& pcs, char type, char message) = 0;
	virtual void SendMsg(const std::tstring& user, char type, char message) = 0;

	virtual ClientData* FindClient(const std::tstring& user) const = 0;
	virtual void DisconnectClient(ClientData* client) = 0;

	virtual void Shutdown() = 0;

	virtual ClientAccess GetClients() const = 0;
	virtual USHORT ClientCount() const = 0;

	virtual Socket GetHostIPv4() const = 0;
	virtual Socket GetHostIPv6() const = 0;

	virtual void Ping(Socket client) = 0;

	virtual bool MaxClients() const = 0;
	virtual bool IsConnected() const = 0;

	virtual UINT MaxDataSize() const = 0;
	virtual UINT MaxCompSize() const = 0;

	virtual int GetCompressionCO() const = 0;

	virtual void* GetObj() const = 0;
};

typedef TCPServInterface::ClientAccess ClientAccess;
typedef TCPServInterface::ClientData ClientData;
typedef void(*const customFunc)(ClientData* data);

typedef TCPServInterface::sfunc sfunc;
typedef TCPServInterface::sfuncP sfuncP;


CAMSNETLIB TCPServInterface* CreateServer(sfunc msgHandler, customFunc conFunc, customFunc disFunc, DWORD nThreads = 4, DWORD nConcThreads = 2, UINT maxDataSize = 8192, USHORT maxCon = 20, int compression = 9, int compressionCO = 512, float pingInterval = 30.0f, void* obj = nullptr);
CAMSNETLIB void DestroyServer(TCPServInterface*& server);
