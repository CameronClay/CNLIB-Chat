//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "TCPServInterface.h"
#include "HeapAlloc.h"
#include "SocketListen.h"
#include "IOCP.h"
#include "OverlappedExt.h"
#include "MemPool.h"
#include "BufSize.h"
#include "WSABufExt.h"

class TCPServ : public TCPServInterface
{
public:
	//sfunc is a message handler, compression is 1-9
	TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads = 4, DWORD nConcThreads = 2, UINT maxDataSize = 8192, USHORT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, void* obj = nullptr);
	TCPServ(TCPServ&& serv);
	~TCPServ();

	struct ClientDataEx : public ClientData
	{
		ClientDataEx(TCPServ& serv, Socket pc, sfunc func, USHORT arrayIndex);
		ClientDataEx(ClientDataEx&& clint);
		ClientDataEx& operator=(ClientDataEx&& data);
		~ClientDataEx();

		TCPServ& serv;
		WSABufExt buff;
		OverlappedExt ol;

		USHORT arrayIndex;
	};

	struct HostSocket
	{
		HostSocket(TCPServ& serv);
		HostSocket(HostSocket&& host);
		HostSocket& operator=(HostSocket&& data);
		~HostSocket();

		void SetAcceptSocket();
		void CloseAcceptSocket();

		TCPServ& serv;
		Socket listen;
		SOCKET accept;
		char* buffer;
		OverlappedExt ol;

		static const DWORD localAddrLen = sizeof(sockaddr_in6) + 16, remoteAddrLen = sizeof(sockaddr_in6) + 16;
	};

	TCPServ& operator=(TCPServ&& serv);

	//Allows connections to the server; should only be called once
	IPv AllowConnections(const LIB_TCHAR* port, IPv ipv = ipboth) override;

	char* GetSendBuffer() override;

	//Used to send data to clients
	//addr parameter functions as both the excluded address, and as a single address, 
	//depending on the value of single
	void SendClientData(const char* data, DWORD nBytes, Socket addr, bool single, CompressionType compType = BESTFIT) override;
	void SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, CompressionType compType = BESTFIT) override;
	void SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType = BESTFIT) override;

	//Send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(Socket pc, bool single, short type, short message) override;
	void SendMsg(Socket* pcs, USHORT nPcs, short type, short message) override;
	void SendMsg(std::vector<Socket>& pcs, short type, short message) override;
	void SendMsg(const std::tstring& user, short type, short message) override;

	ClientData* FindClient(const std::tstring& user) const override;
	void DisconnectClient(ClientData* client, bool unexpected = true) override;

	void Shutdown() override;

	void AddClient(Socket pc);
	void RemoveClient(ClientDataEx* client, bool unexpected = true);

	void KeepAlive() override;
	void KeepAlive(Socket client) override;

	void RunConFunc(ClientData* client);
	void RunDisFunc(ClientData* client, bool unexpected);

	ClientAccess GetClients() const override;
	USHORT ClientCount() const override;

	Socket GetHostIPv4() const override;
	Socket GetHostIPv6() const override;

	void SetKeepAliveInterval(float interval) override;
	float GetKeepAliveInterval() const override;

	bool MaxClients() const override;
	void* GetObj() const override;
	int GetCompression() const;
	int GetCompressionCO() const override;
	bool IsConnected() const override;

	UINT MaxDataSize() const override;
	UINT MaxCompSize() const override;
	UINT MaxBufferSize() const;

	IOCP& GetIOCP();

	MemPool& GetRecvBuffPool();

	WSABufExt CreateSendBuffer(DWORD nBytesDecomp, char* buffer, OpType opType, CompressionType compType = BESTFIT);
	void FreeSendBuffer(WSABufExt buff, OpType opType);
	void FreeSendOverlapped(OverlappedSend* ol);
private:
	void SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, OpType opType, CompressionType compType);
	void SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, OpType opType, CompressionType compType);

	HostSocket ipv4Host, ipv6Host; //host/listener sockets
	ClientDataEx** clients; //array of clients
	USHORT nClients; //number of current connected clients
	IOCP iocp; //IO completion port
	sfunc function; //used to intialize what clients default function/msghandler is
	void* obj; //passed to function/msgHandler for oop programming
	ConFunc conFunc; //function called when connect
	DisconFunc disFunc; //function called when disconnect occurs
	CRITICAL_SECTION clientSect; //used for synchonization
	const UINT maxDataSize, maxCompSize, maxBufferSize; //maximum packet size to send or recv, maximum compressed data size
	const int compression, compressionCO; //compression server sends packets at
	const USHORT maxCon; //max clients
	float keepAliveInterval; //interval at which server KeepAlives(technically is a keep alive message that sends data) clients
	KeepAliveHandler* keepAliveHandler; //handles all KeepAlives to client, to prevent timeout
	MemPool clientPool, recvBuffPool; //Used to help speed up allocation of client resources
	MemPool sendOlPoolSingle, sendOlPoolAll; //Used to help speed up allocation of structures needed to send Ol data
	MemPool sendDataPool, sendMsgPool; //Used to help speed up allocation of send buffers
};

typedef TCPServ::ClientDataEx ClientDataEx;
typedef TCPServ::HostSocket HostSocket;
