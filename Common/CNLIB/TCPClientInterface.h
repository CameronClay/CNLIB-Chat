//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Typedefs.h"
#include "CompressionTypes.h"
#include "Socket.h"
#include "MsgStream.h"
#include "StreamAllocInterface.h"
#include "SocketOptions.h"

class TCPClientInterface;
typedef void(*const dcfunc)(bool unexpected);
typedef void(*cfunc)(TCPClientInterface& client, MsgStreamReader streamReader);
typedef void(**const cfuncP)(TCPClientInterface& client, MsgStreamReader streamReader);

class CAMSNETLIB TCPClientInterface : public StreamAllocInterface
{
public:
	virtual bool Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6 = false, float timeOut = 5.0f) = 0;

	virtual void Shutdown() = 0;
	virtual void Disconnect() = 0;

	virtual bool RecvServData(DWORD nThreads = 1, DWORD nConcThreads = 1) = 0;

	virtual bool SendServData(const BuffSendInfo& buffSendInfo, DWORD nBytes, BuffAllocator* alloc = nullptr) = 0;
	virtual bool SendServData(const MsgStreamWriter& streamWriter, BuffAllocator* alloc = nullptr) = 0;

	virtual void SendMsg(short type, short message) = 0;
	virtual void SendMsg(const std::tstring& user, short type, short message) = 0;

	virtual void SetFunction(cfunc function) = 0;

	virtual bool IsConnected() const = 0;

	virtual UINT GetOpCount() const = 0;
	virtual UINT GetMaxSendOps() const = 0;

	virtual const SocketOptions GetSockOpts() const = 0;

	virtual Socket GetHost() const = 0;
	virtual void* GetObj() const = 0;
};

CAMSNETLIB TCPClientInterface* CreateClient(cfunc msgHandler, dcfunc disconFunc, UINT maxSendOps = 5, UINT maxDataBuffSize = 4096, UINT olCount = 10, UINT sendBuffCount = 35, UINT sendCompBuffCount = 15, UINT sendMsgBuffCount = 10, UINT maxCon = 20, int compression = 9, int compressionCO = 512, float keepAliveInterval = 30.0f, SocketOptions sockOpts = SocketOptions(), void* obj = nullptr);
CAMSNETLIB void DestroyClient(TCPClientInterface*& client);