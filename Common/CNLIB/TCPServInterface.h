//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Typedefs.h"
#include "Socket.h"
#include "IPV.h"
#include "CompressionTypes.h"

class CAMSNETLIB TCPServInterface
{
public:
	struct ClientData;
	typedef void(*sfunc)(TCPServInterface& serv, ClientData* const client, const BYTE* data, DWORD nBytes);
	typedef void(**const sfuncP)(TCPServInterface& serv, ClientData* const client, const BYTE* data, DWORD nBytes);
	typedef bool(*ConCondition)(TCPServInterface& serv, const Socket& socket);

	struct CAMSNETLIB ClientData
	{
		ClientData(Socket pc, sfunc func);
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
		ClientData* operator+(UINT amount);
		ClientData* operator-(UINT amount);
		ClientData* operator[](UINT index);
	private:
		const ClientData** clients;
	};

	virtual IPv AllowConnections(const LIB_TCHAR* port, ConCondition connectionCondition, IPv ipv = ipboth) = 0;

	virtual char* GetSendBuffer() = 0;

	virtual void SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, CompressionType compType = BESTFIT) = 0;
	virtual void SendClientData(const char* data, DWORD nBytes, Socket* pcs, UINT nPcs, CompressionType compType = BESTFIT) = 0;
	virtual void SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType = BESTFIT) = 0;

	virtual void SendMsg(Socket exAddr, bool single, short type, short message) = 0;
	virtual void SendMsg(Socket* pcs, UINT nPcs, short type, short message) = 0;
	virtual void SendMsg(std::vector<Socket>& pcs, short type, short message) = 0;
	virtual void SendMsg(const std::tstring& user, short type, short message) = 0;

	virtual ClientData* FindClient(const std::tstring& user) const = 0;
	virtual void DisconnectClient(ClientData* client) = 0;

	virtual void Shutdown() = 0;

	virtual ClientAccess GetClients() const = 0;
	virtual UINT ClientCount() const = 0;
	virtual UINT MaxClientCount() const = 0;

	virtual Socket GetHostIPv4() const = 0;
	virtual Socket GetHostIPv6() const = 0;

	virtual bool MaxClients() const = 0;
	virtual bool IsConnected() const = 0;
	virtual bool NoDelay() const = 0;

	virtual UINT MaxDataSize() const = 0;
	virtual UINT MaxCompSize() const = 0;
	virtual UINT GetOpCount() const = 0;

	virtual int GetCompressionCO() const = 0;

	virtual void* GetObj() const = 0;
};

typedef TCPServInterface::ConCondition ConCondition;

typedef TCPServInterface::ClientAccess ClientAccess;
typedef TCPServInterface::ClientData ClientData;

typedef TCPServInterface::sfunc sfunc;
typedef TCPServInterface::sfuncP sfuncP;

typedef void(*const ConFunc)(ClientData* data);
typedef void(*const DisconFunc)(ClientData* data, bool unexpected);


CAMSNETLIB TCPServInterface* CreateServer(sfunc msgHandler, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads = 4, DWORD nConcThreads = 2, UINT maxDataSize = 8192, UINT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, bool noDelay = false, void* obj = nullptr);
CAMSNETLIB void DestroyServer(TCPServInterface*& server);
