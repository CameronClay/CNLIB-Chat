//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "CNLIB/TCPServInterface.h"
#include "CNLIB/IOCP.h"
#include "CNLIB/MemPool.h"
#include "CNLIB/KeepAlive.h"
#include "CNLIB/WSABufExt.h"
#include "CNLIB/OverlappedExt.h"
#include "CNLIB/HeapAlloc.h"
#include "InterlockedCounter.h"
#include "BufSendAlloc.h"

class TCPServ : public TCPServInterface, public KeepAliveHI
{
public:
	//sfunc is a message handler, compression is 1-9
	//a value of 0.0f of ping interval means dont keepalive at all
	TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads = 4, DWORD nConcThreads = 2, UINT maxPCSendOps = 5, UINT maxDataSize = 8192, UINT singleOlPCCount = 5, UINT allOlCount = 30, UINT sendBuffCount = 40, UINT sendMsgBuffCount = 20, UINT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, SocketOptions sockOpts = SocketOptions(), void* obj = nullptr);
	TCPServ(TCPServ&& serv);
	~TCPServ();

	struct ClientDataEx : public ClientData
	{
		ClientDataEx(TCPServ& serv, Socket pc, sfunc func, UINT arrayIndex);
		ClientDataEx(ClientDataEx&& clint);
		ClientDataEx& operator=(ClientDataEx&& data);
		~ClientDataEx();

		TCPServ& serv;
		WSABufRecv buff;
		OverlappedExt ol;
		UINT arrayIndex;
		InterlockedCounter opCount;
		MemPoolSync<HeapAllocator> olPool;
		std::queue<OverlappedSendSingle*> opsPending;

		//int for alignment
		enum State : int
		{
			running, closing
		} state;
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
	IPv AllowConnections(const LIB_TCHAR* port, ConCondition connectionCondition, IPv ipv = ipboth) override;

	char* GetSendBuffer() override;
	MsgStreamWriter CreateOutStream(short type, short msg) override;
	const BufferOptions GetBufferOptions() const override;

	//Used to send data to clients
	//addr parameter functions as both the excluded address, and as a single address, 
	//depending on the value of single
	bool SendClientData(const char* data, DWORD nBytes, ClientData* exClient, bool single, CompressionType compType = BESTFIT) override;
	bool SendClientData(const char* data, DWORD nBytes, ClientData** clients, UINT nClients, CompressionType compType = BESTFIT) override;
	bool SendClientData(const char* data, DWORD nBytes, std::vector<ClientData*>& pcs, CompressionType compType = BESTFIT) override;

	bool SendClientData(MsgStreamWriter streamWriter, ClientData* exClient, bool single, CompressionType compType = BESTFIT) override;
	bool SendClientData(MsgStreamWriter streamWriter, ClientData** clients, UINT nClients, CompressionType compType = BESTFIT) override;
	bool SendClientData(MsgStreamWriter streamWriter, std::vector<ClientData*>& pcs, CompressionType compType = BESTFIT) override;

	//Send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(ClientData* exClient, bool single, short type, short message) override;
	void SendMsg(ClientData** clients, UINT nClients, short type, short message) override;
	void SendMsg(std::vector<ClientData*>& pcs, short type, short message) override;
	void SendMsg(const std::tstring& user, short type, short message) override;

	ClientData* FindClient(const std::tstring& user) const override;
	void DisconnectClient(ClientData* client) override;

	void Shutdown() override;

	void AddClient(Socket pc);
	void RemoveClient(ClientDataEx* client, bool unexpected = true);

	void KeepAlive() override;

	void RunConFunc(ClientData* client);
	void RunDisFunc(ClientData* client, bool unexpected);

	ClientAccess GetClients() const override;
	UINT ClientCount() const override;
	UINT MaxClientCount() const override;

	Socket GetHostIPv4() const override;
	Socket GetHostIPv6() const override;

	void SetKeepAliveInterval(float interval) override;
	float GetKeepAliveInterval() const override;

	bool MaxClients() const override;
	bool IsConnected() const override;

	const SocketOptions GetSockOpts() const override;

	void* GetObj() const override;

	UINT GetOpCount() const override;

	UINT SingleOlPCCount() const override;
	UINT GetMaxPcSendOps() const override;

	IOCP& GetIOCP();

	MemPool<HeapAllocator>& GetRecvBuffPool();

	void FreeSendOlInfo(OverlappedSendInfo* ol);
	void FreeSendOlSingle(ClientDataEx& client, OverlappedSendSingle* ol);

	void AcceptConCR(HostSocket& host, OverlappedExt* ol);
	void RecvDataCR(DWORD bytesTrans, ClientDataEx& cd);
	void SendDataCR(ClientDataEx& cd, OverlappedSend* ol);
	void SendDataSingleCR(ClientDataEx& cd, OverlappedSendSingle* ol);
	void CleanupAcceptEx(HostSocket& host);
private:
	bool BindHost(HostSocket& host, bool ipv6, const LIB_TCHAR* port);
	bool SendClientData(const char* data, DWORD nBytes, ClientDataEx* exClient, bool single, bool msg, CompressionType compType);
	bool SendClientData(const char* data, DWORD nBytes, ClientDataEx** clients, UINT nClients, bool msg, CompressionType compType);

	bool SendClientData(const WSABufSend& sendBuff, ClientDataEx* exClient, bool single);
	bool SendClientData(const WSABufSend& sendBuff, ClientDataEx** clients, UINT nClients);
	bool SendClientSingle(ClientDataEx& clint, OverlappedSendSingle* ol, bool popQueue = false);

	HostSocket ipv4Host, ipv6Host; //host/listener sockets
	ClientDataEx** clients; //array of clients
	UINT nClients; //number of current connected clients
	IOCP iocp; //IO completion port
	sfunc function; //used to intialize what clients default function/msghandler is
	ConFunc conFunc; //function called when connect
	ConCondition connectionCondition; //condition for accepting connections
	DisconFunc disFunc; //function called when disconnect occurs
	CRITICAL_SECTION clientSect; //used for synchonization
	const UINT singleOlPCCount, maxPCSendOps; //number of preallocated per client ol structs, max per client concurent send operations
	const UINT maxCon; //max clients
	float keepAliveInterval; //interval at which server keepalives clients
	KeepAliveHandler* keepAliveHandler; //handles all KeepAlives to client, to prevent timeout
	BufSendAlloc bufSendAlloc;
	MemPool<HeapAllocator> clientPool, recvBuffPool; //Used to help speed up allocation of client resources
	MemPoolSync<HeapAllocator> sendOlInfoPool, sendOlPoolAll; //Used to help speed up allocation of structures needed to send Ol data, single pool is backup for per client pool
	InterlockedCounter opCounter; //Used to keep track of number of asynchronous operations
	HANDLE shutdownEv; //Set when opCounter reaches 0, to notify shutdown it is okay to close iocp
	void* obj; //passed to function/msgHandler for oop programming
	SocketOptions sockOpts;
};

typedef TCPServ::ClientDataEx ClientDataEx;
typedef TCPServ::HostSocket HostSocket;
