//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "CNLIB/TCPServInterface.h"
#include "CNLIB/IOCP.h"
#include "CNLIB/MemPool.h"
#include "KeepAlive.h"
#include "WSABufExt.h"
#include "OverlappedExt.h"
#include "CNLIB/HeapAlloc.h"
#include "BufSendAlloc.h"
#include "RecvHandler.h"
#include "RecvObserverI.h"

class TCPServ : public TCPServInterface, public KeepAliveHI, public RecvObserverI
{
public:
	//sfunc is a message handler, compression is 1-9
	//a value of 0.0f of ping interval means dont keepalive at all
	TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, UINT maxPCSendOps = 5, UINT maxDataBuffSize = 4096, UINT singleOlPCCount = 5, UINT allOlCount = 30, UINT sendBuffCount = 35, UINT sendCompBuffCount = 15, UINT sendMsgBuffCount = 10, UINT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, SocketOptions sockOpts = SocketOptions(), void* obj = nullptr);
	TCPServ(TCPServ&& serv);
	~TCPServ();

	struct ClientDataEx : public ClientData
	{
		ClientDataEx(TCPServ& serv, Socket pc, sfunc func, UINT arrayIndex);
		ClientDataEx(ClientDataEx&& clint);
		ClientDataEx& operator=(ClientDataEx&& data);
		~ClientDataEx();

		TCPServ& serv;
		OverlappedExt ol;
		UINT arrayIndex;
		std::atomic<UINT> opCount;
		MemPoolSync<HeapAllocator> olPool;
		RecvHandler recvHandler;
		std::queue<OverlappedSendSingle*> opsPending;

		bool DecRefCount();
		void FreePendingOps();
		void Cleanup();
		void Reset(const Socket& pc, UINT arrayIndex);

		//int for alignment
		enum State : int
		{
			running, closing
		} state;
	};

	class HostSocket
	{
	public:
		class AcceptData
		{
		public:
			struct OverlappedAccept : OverlappedExt
			{
				OverlappedAccept(AcceptData* acceptData)
					:
					OverlappedExt(OpType::accept),
					acceptData(nullptr)
				{}

				AcceptData* acceptData;
			};

			AcceptData();
			AcceptData(AcceptData&& host);
			AcceptData& operator=(AcceptData&& data);

			void Initalize(HostSocket* hostSocket);
			void Cleanup();

			void ResetAccept();
			bool QueueAccept();
			void AcceptConCR();

			SOCKET GetAccept() const;
			HostSocket* GetHostSocket() const;
			OverlappedAccept* GetOl();
			void GetAcceptExAddrs(sockaddr** local, int* localLen, sockaddr** remote, int* remoteLen);
		private:
			HostSocket* hostSocket;
			SOCKET accept;
			char* buffer;
			OverlappedAccept ol;
		};

		HostSocket(TCPServ& serv);
		HostSocket(HostSocket&& host);
		HostSocket& operator=(HostSocket&& data);

		void Initalize(UINT nConcAccepts);
		void Cleanup();

		bool Bind(const LIB_TCHAR* port, bool ipv6);
		bool QueueAccept();
		bool LinkIOCP(IOCP& iocp);
		void Disconnect();

		TCPServ& GetServ();
		const Socket& GetListen() const;

		static const DWORD localAddrLen = sizeof(sockaddr_in6) + 16, remoteAddrLen = sizeof(sockaddr_in6) + 16;

	private:
		TCPServ& serv;
		Socket listen;

		AcceptData* acceptData;
		UINT nConcAccepts;
	};

	typedef HostSocket::AcceptData::OverlappedAccept OverlappedAccept;

	TCPServ& operator=(TCPServ&& serv);

	//Allows connections to the server; should only be called once
	IPv AllowConnections(const LIB_TCHAR* port, ConCondition connectionCondition, DWORD nThreads = 4, DWORD nConcThreads = 2, IPv ipv = ipv4, UINT nConcAccepts = 4) override;

	BuffSendInfo GetSendBuffer(DWORD hiByteEstimate, CompressionType compType = BESTFIT) override;
	BuffSendInfo GetSendBuffer(BuffAllocator* alloc, DWORD nBytes, CompressionType compType = BESTFIT) override;

	MsgStreamWriter CreateOutStream(DWORD hiByteEstimate, short type, short msg, CompressionType compType = BESTFIT) override;
	MsgStreamWriter CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg, CompressionType compType = BESTFIT) override;

	const BufferOptions GetBufferOptions() const override;

	//Used to send data to clients
	//addr parameter functions as both the excluded address, and as a single address, 
	//depending on the value of single
	bool SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientData* exClient, bool single, BuffAllocator* alloc = nullptr) override;
	bool SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientData** clients, UINT nClients, BuffAllocator* alloc = nullptr) override;
	bool SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, std::vector<ClientData*>& pcs, BuffAllocator* alloc = nullptr) override;

	bool SendClientData(const MsgStreamWriter& streamWriter, ClientData* exClient, bool single, BuffAllocator* alloc = nullptr) override;
	bool SendClientData(const MsgStreamWriter& streamWriter, ClientData** clients, UINT nClients, BuffAllocator* alloc = nullptr) override;
	bool SendClientData(const MsgStreamWriter& streamWriter, std::vector<ClientData*>& pcs, BuffAllocator* alloc = nullptr) override;

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

	const Socket GetHostIPv4() const override;
	const Socket GetHostIPv6() const override;

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

	void FreeSendOlInfo(OverlappedSendInfo* ol);
	void FreeSendOlSingle(ClientDataEx& client, OverlappedSendSingle* ol);

	void AcceptConCR(HostSocket::AcceptData& acceptData);
	void RecvDataCR(DWORD bytesTrans, ClientDataEx& cd);
	void SendDataCR(ClientDataEx& cd, OverlappedSend* ol);
	void SendDataSingleCR(ClientDataEx& cd, OverlappedSendSingle* ol);
	void CleanupAcceptEx(HostSocket::AcceptData& acceptData);
private:
	void OnNotify(char* data, DWORD nBytes, void* cd) override;

	bool BindHost(HostSocket& host, const LIB_TCHAR* port, bool ipv6, UINT nConcAccepts);
	bool SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientDataEx* exClient, bool single, bool msg, BuffAllocator* alloc = nullptr);
	bool SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientDataEx** clients, UINT nClients, bool msg, BuffAllocator* alloc = nullptr);

	bool SendClientData(const WSABufSend& sendBuff, ClientDataEx* exClient, bool single);
	bool SendClientData(const WSABufSend& sendBuff, ClientDataEx** clients, UINT nClients);
	bool SendClientSingle(ClientDataEx& clint, OverlappedSendSingle* ol, bool popQueue = false);

	HostSocket ipv4Host, ipv6Host; //host/listener sockets
	ClientDataEx** clients; //array of clients
	UINT nClients; //number of current connected clients
	IOCP* iocp; //IO completion port
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
	MemPool<HeapAllocator> clientPool;
	MemPoolSync<PageAllignAllocator> sendOlInfoPool, sendOlPoolAll; //Used to help speed up allocation of structures needed to send Ol data, single pool is backup for per client pool
	std::atomic<UINT> opCounter; //Used to keep track of number of asynchronous operations
	HANDLE shutdownEv; //Set when opCounter reaches 0, to notify shutdown it is okay to close iocp
	void* obj; //passed to function/msgHandler for oop programming
	SocketOptions sockOpts;
};

typedef TCPServ::ClientDataEx ClientDataEx;
typedef TCPServ::HostSocket HostSocket;
