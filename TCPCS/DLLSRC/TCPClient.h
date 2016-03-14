//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "CNLIB/TCPClientInterface.h"
#include "CNLIB/IOCP.h"
#include "CNLIB/MemPool.h"
#include "CNLIB/KeepAlive.h"
#include "CNLIB/WSABufExt.h"
#include "CNLIB/OverlappedExt.h"
#include "CNLIB/HeapAlloc.h"
#include "InterlockedCounter.h"

class TCPClient : public TCPClientInterface, public KeepAliveHI
{
public:
	//cfunc is a message handler, compression 1-9
	TCPClient(cfunc func, dcfunc disconFunc, DWORD nThreads = 4, DWORD nConcThreads = 2, UINT maxSendOps = 5, UINT maxDataSize = 8192, UINT olCount = 10, UINT sendBuffCount = 40, UINT sendMsgBuffCount = 20, UINT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, bool useOwnBuf = true, bool noDelay = false, void* obj = nullptr);
	TCPClient(TCPClient&& client);
	~TCPClient();

	TCPClient& operator=(TCPClient&& client);


	bool Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6 = false, float timeOut = 5.0f);
	void Disconnect();
	void Shutdown();

	char* GetSendBuffer() override;

	MsgStreamWriter CreateOutStream(short type, short msg) override;

	bool SendServData(const char* data, DWORD nBytes, CompressionType compType = BESTFIT) override;
	bool SendServData(MsgStreamWriter streamWriter, CompressionType compType = BESTFIT) override;

	//Should only be called once to intialy create receving thread
	bool RecvServData() override;

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

	UINT MaxDataSize() const override;
	UINT MaxCompSize() const override;
	UINT GetOpCount() const override;

	UINT GetMaxSendOps() const override;

	int GetCompression() const override;
	int GetCompressionCO() const override;

	bool NoDelay() const override;
	bool UseOwnBuf() const override;

	dcfunc GetDisfunc() const;
	bool IsConnected() const override;

	void FreeSendBuffer(WSABufSend& buff);
	void FreeSendOl(OverlappedSendSingle* ol);

	void RecvDataCR(DWORD bytesTrans,OverlappedExt* ol);
	void SendDataCR(OverlappedSendSingle* ol);

	void CleanupRecvData();
private:
	WSABufSend CreateSendBuffer(DWORD nBytesDecomp, char* buffer, bool msg, CompressionType compType = BESTFIT);

	bool SendServData(const char* data, DWORD nBytes, bool msg, CompressionType compType = BESTFIT);
	bool SendServData(OverlappedSendSingle* ol, bool popQueue = false);

	Socket host; //server/host you are connected to
	cfunc function; //function/msgHandler
	dcfunc disconFunc; //function called when disconnect occurs
	IOCP iocp; //IO completion port
	const UINT maxDataSize, maxCompSize; //maximum packet size to send or recv, maximum compressed data size, number of preallocated sendbuffers
	const UINT maxSendOps; //max send operations
	const int compression, compressionCO; //compression server sends packets at
	bool unexpectedShutdown; //passed to disconnect handler
	WSABufRecv recvBuff; //recv buffer
	OverlappedExt recvOl; //recv overlapped
	float keepAliveInterval; //interval at which client KeepAlives server
	KeepAliveHandler* keepAliveHandler; //handles all KeepAlives(technically is a keep alive message that sends data) to server, to prevent timeout
	MemPoolSync<HeapAllocator> sendDataPool, sendMsgPool; //Used to help speed up allocation of send buffers
	MemPoolSync<HeapAllocator> olPool; //Used to help speed up allocation of overlapped structures
	std::queue<OverlappedSendSingle*> opsPending; //Used to store pending ops
	InterlockedCounter opCounter; //Used to keep track of number of asynchronous operations
	HANDLE shutdownEv; //Set when opCounter reaches 0, to notify shutdown it is okay to close iocp
	bool noDelay; //Used to enable/disable the nagle algorithm
	bool useOwnBuf; //If true calls setsockopt with SO_SENDBUF and 0, to force it to use your own buffer
	void* obj; //passed to function/msgHandler for oop programming
};
