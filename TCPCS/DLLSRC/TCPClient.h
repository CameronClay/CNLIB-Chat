//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "CNLIB/TCPClientInterface.h"
#include "CNLIB/IOCP.h"
#include "CNLIB/MemPool.h"
#include "KeepAlive.h"
#include "WSABufExt.h"
#include "OverlappedExt.h"
#include "CNLIB/HeapAlloc.h"
#include "BufSendAlloc.h"
#include "RecvHandler.h"
#include "RecvObserverI.h"
#include "CNLIB/CritLock.h"

class TCPClient : public TCPClientInterface, public KeepAliveHI, public RecvObserverI
{
public:
	//cfunc is a message handler, compression 1-9
	TCPClient(cfunc func, dcfunc disconFunc, UINT maxSendOps = 5, UINT maxDataBuffSize = 4096, UINT olCount = 10, UINT sendBuffCount = 35, UINT sendCompBuffCount = 15, UINT sendMsgBuffCount = 10, UINT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, SocketOptions sockOpts = SocketOptions(), void* obj = nullptr);
	TCPClient(TCPClient&& client);
	~TCPClient();

	TCPClient& operator=(TCPClient&& client);

	bool Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6 = false, float timeOut = 5.0f);
	void Disconnect();
	void Shutdown();

	BuffSendInfo GetSendBuffer(DWORD hiByteEstimate, CompressionType compType = BESTFIT) override;
	BuffSendInfo GetSendBuffer(BuffAllocator* alloc, DWORD nBytes, CompressionType compType = BESTFIT) override;

	MsgStreamWriter CreateOutStream(DWORD hiByteEstimate, short type, short msg, CompressionType compType = BESTFIT) override;
	MsgStreamWriter CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg, CompressionType compType = BESTFIT) override;

	const BufferOptions GetBufferOptions() const override;

	bool SendServData(const BuffSendInfo& buffSendInfo, DWORD nBytes, BuffAllocator* alloc = nullptr) override;
	bool SendServData(const MsgStreamWriter& streamWriter, BuffAllocator* alloc = nullptr) override;

	//Should only be called once to intialy create receving thread
	bool RecvServData(DWORD nThreads = 4, DWORD nConcThreads = 2) override;

	//send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(short type, short message) override;
	void SendMsg(const std::tstring& user, short type, short message) override;

	void KeepAlive() override;

	void SetKeepAliveInterval(float interval) override;
	float GetKeepAliveInterval() const override;

	void SetFunction(cfunc function) override;

	void RunDisconFunc();
	void SetShutdownReason(bool unexpected);

	void* GetObj() const override;
	cfuncP GetFunction();

	Socket GetHost() const override;

	UINT GetOpCount() const override;
	UINT GetMaxSendOps() const override;

	const SocketOptions GetSockOpts() const override;

	dcfunc GetDisfunc() const;
	bool IsConnected() const override;

	void FreeSendOl(OverlappedSendSingle* ol);

	bool RecvDataCR(DWORD bytesTrans);
	void SendDataCR(OverlappedSendSingle* ol);

	void CleanupRecvData();
private:
	void OnNotify(char* data, DWORD nBytes, void*) override;

	bool SendServData(const BuffSendInfo& buffSendInfo, DWORD nBytes, bool msg, BuffAllocator* alloc = nullptr);
	bool SendServData(const WSABufSend& sendBuff, bool popQueue = false);
	bool SendServData(OverlappedSendSingle* ol, bool popQueue = false);

	Socket host; //server/host you are connected to
	cfunc function; //function/msgHandler
	dcfunc disconFunc; //function called when disconnect occurs
	IOCP* iocp; //IO completion port
	const UINT maxSendOps; //max send operations
	bool unexpectedShutdown; //passed to disconnect handler
	float keepAliveInterval; //interval at which client KeepAlives server
	KeepAliveHandler* keepAliveHandler; //handles all KeepAlives(technically is a keep alive message that sends data) to server, to prevent timeout
	BufSendAlloc bufSendAlloc;
	RecvHandler recvHandler;
	MemPoolSync<HeapAllocator> olPool; //Used to help speed up allocation of overlapped structures
	std::queue<OverlappedSendSingle*> opsPending; //Used to store pending ops
	CritLock queueLock;
	std::atomic<UINT> opCounter; //Used to keep track of number of asynchronous operations
	HANDLE shutdownEv; //Set when opCounter reaches 0, to notify shutdown it is okay to close iocp
	SocketOptions sockOpts;
	void* obj; //passed to function/msgHandler for oop programming
};
