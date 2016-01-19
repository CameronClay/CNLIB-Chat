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
#include "InterlockedCounter.h"

class TCPServ : public TCPServInterface, public KeepAliveHI
{
public:
	//sfunc is a message handler, compression is 1-9
	TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads = 4, DWORD nConcThreads = 2, UINT maxDataSize = 8192, UINT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, bool noDelay = false, void* obj = nullptr);
	TCPServ(TCPServ&& serv);
	~TCPServ();

	struct ClientDataEx : public ClientData
	{
		ClientDataEx(TCPServ& serv, Socket pc, sfunc func, UINT arrayIndex);
		ClientDataEx(ClientDataEx&& clint);
		ClientDataEx& operator=(ClientDataEx&& data);
		~ClientDataEx();

		TCPServ& serv;
		WSABufExt buff;
		OverlappedExt ol;
		UINT arrayIndex;

		//int for allignment
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

	//Used to send data to clients
	//addr parameter functions as both the excluded address, and as a single address, 
	//depending on the value of single
	void SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, CompressionType compType = BESTFIT) override;
	void SendClientData(const char* data, DWORD nBytes, Socket* pcs, UINT nPcs, CompressionType compType = BESTFIT) override;
	void SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType = BESTFIT) override;

	//Send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(Socket exAddr, bool single, short type, short message) override;
	void SendMsg(Socket* pcs, UINT nPcs, short type, short message) override;
	void SendMsg(std::vector<Socket>& pcs, short type, short message) override;
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
	UINT GetOpCount() const override;
	UINT MaxBufferSize() const;

	InterlockedCounter& GetOpCounter();

	IOCP& GetIOCP();

	MemPool& GetRecvBuffPool();

	WSABufExt CreateSendBuffer(DWORD nBytesDecomp, char* buffer, OpType opType, CompressionType compType = BESTFIT);
	void FreeSendBuffer(WSABufExt buff, OpType opType);
	void FreeSendOverlapped(OverlappedSend* ol);

	void RecvDataCR(DWORD bytesTrans, ClientDataEx& cd, OverlappedExt* ol);
	void AcceptConCR(HostSocket& host, OverlappedExt* ol);
	void SendDataCR(ClientDataEx& cd, OverlappedSend* ol);
	void CleanupAcceptEx(HostSocket& host);
private:
	void SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, OpType opType, CompressionType compType);
	void SendClientData(const char* data, DWORD nBytes, Socket* pcs, UINT nPcs, OpType opType, CompressionType compType);

	HostSocket ipv4Host, ipv6Host; //host/listener sockets
	ClientDataEx** clients; //array of clients
	UINT nClients; //number of current connected clients
	IOCP iocp; //IO completion port
	sfunc function; //used to intialize what clients default function/msghandler is
	void* obj; //passed to function/msgHandler for oop programming
	ConFunc conFunc; //function called when connect
	ConCondition connectionCondition; //condition for accepting connections
	DisconFunc disFunc; //function called when disconnect occurs
	CRITICAL_SECTION clientSect; //used for synchonization
	const UINT maxDataSize, maxCompSize, maxBufferSize; //maximum packet size to send or recv, maximum compressed data size
	const int compression, compressionCO; //compression server sends packets at
	const USHORT maxCon; //max clients
	float keepAliveInterval; //interval at which server KeepAlives(technically is a keep alive message that sends data) clients
	KeepAliveHandler* keepAliveHandler; //handles all KeepAlives to client, to prevent timeout
	MemPool clientPool, recvBuffPool; //Used to help speed up allocation of client resources
	MemPoolSync sendOlPoolSingle, sendOlPoolAll; //Used to help speed up allocation of structures needed to send Ol data
	MemPoolSync sendDataPool, sendMsgPool; //Used to help speed up allocation of send buffers
	InterlockedCounter opCounter; //Used to keep track of number of asynchronous operations
	HANDLE shutdownEv; //Set when opCounter reaches 0, to notify shutdown it is okay to close iocp
	bool noDelay; //Used to enable/disable the nagle algorithim, stored at end to keep allignment
};

typedef TCPServ::ClientDataEx ClientDataEx;
typedef TCPServ::HostSocket HostSocket;
