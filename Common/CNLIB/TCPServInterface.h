//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Typedefs.h"
#include "Socket.h"
#include "IPV.h"
#include "CompressionTypes.h"
#include "MsgStream.h"
#include "StreamAllocInterface.h"
#include "SocketOptions.h"
#include "BuffSendInfo.h"

class CAMSNETLIB TCPServInterface : public StreamAllocInterface
{
public:
	struct ClientData;
	typedef void(*sfunc)(TCPServInterface& serv, ClientData* const client, MsgStreamReader streamReader);
	typedef void(**const sfuncP)(TCPServInterface& serv, ClientData* const client, const BYTE* data, DWORD nBytes);
	typedef bool(*ConCondition)(TCPServInterface& serv, const Socket& socket);

	struct CAMSNETLIB ClientData
	{
		ClientData(Socket pc, sfunc func);
		ClientData(ClientData&& clint);
		ClientData& operator=(ClientData&& data);

		UINT GetOpCount() const;

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
		ClientData* operator[](UINT index);
	private:
		ClientData** const clients;
	};

	virtual IPv AllowConnections(const LIB_TCHAR* port, ConCondition connectionCondition, DWORD nThreads = 4, DWORD nConcThreads = 2, IPv ipv = ipboth, UINT nConcAccepts = 1) = 0;

	virtual bool SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientData* exClient, bool single, BuffAllocator* alloc = nullptr) = 0;
	virtual bool SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientData** clients, UINT nClients, BuffAllocator* alloc = nullptr) = 0;
	virtual bool SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, std::vector<ClientData*>& pcs, BuffAllocator* alloc = nullptr) = 0;

	virtual bool SendClientData(const MsgStreamWriter& streamWriter, ClientData* exClient, bool single, BuffAllocator* alloc = nullptr) = 0;
	virtual bool SendClientData(const MsgStreamWriter& streamWriter, ClientData** clients, UINT nClients, BuffAllocator* alloc = nullptr) = 0;
	virtual bool SendClientData(const MsgStreamWriter& streamWriter, std::vector<ClientData*>& pcs, BuffAllocator* alloc = nullptr) = 0;

	virtual void SendMsg(ClientData* exClient, bool single, short type, short message) = 0;
	virtual void SendMsg(ClientData** clients, UINT nClients, short type, short message) = 0;
	virtual void SendMsg(std::vector<ClientData*>& pcs, short type, short message) = 0;
	virtual void SendMsg(const std::tstring& user, short type, short message) = 0;

	virtual ClientData* FindClient(const std::tstring& user) const = 0;
	virtual void DisconnectClient(ClientData* client) = 0;

	virtual void Shutdown() = 0;

	virtual ClientAccess GetClients() const = 0;
	virtual UINT ClientCount() const = 0;
	virtual UINT MaxClientCount() const = 0;

	virtual const Socket GetHostIPv4() const = 0;
	virtual const Socket GetHostIPv6() const = 0;

	virtual bool MaxClients() const = 0;
	virtual bool IsConnected() const = 0;

	virtual UINT SingleOlPCCount() const = 0;
	virtual UINT GetMaxPcSendOps() const = 0;

	virtual UINT GetOpCount() const = 0;

	virtual const SocketOptions GetSockOpts() const = 0;

	virtual void* GetObj() const = 0;
};

typedef TCPServInterface::ConCondition ConCondition;

typedef TCPServInterface::ClientAccess ClientAccess;
typedef TCPServInterface::ClientData ClientData;

typedef TCPServInterface::sfunc sfunc;
typedef TCPServInterface::sfuncP sfuncP;

typedef void(*const ConFunc)(ClientData* data);
typedef void(*const DisconFunc)(ClientData* data, bool unexpected);


CAMSNETLIB TCPServInterface* CreateServer(sfunc msgHandler, ConFunc conFunc, DisconFunc disFunc, UINT maxPCSendOps = 5, UINT maxDataBuffSize = 4096, UINT singleOlPCCount = 30, UINT allOlCount = 30, UINT sendBuffCount = 35, UINT sendCompBuffCount = 15, UINT sendMsgBuffCount = 10, UINT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, SocketOptions sockOpts = SocketOptions(), void* obj = nullptr);
CAMSNETLIB void DestroyServer(TCPServInterface*& server);
