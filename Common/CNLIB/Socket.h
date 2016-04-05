//Copyright (c) <2015> <Cameron Clay>

#pragma once

#include "Typedefs.h"
#include "SocketInfo.h"

CAMSNETLIB int InitializeNetworking();
CAMSNETLIB int CleanupNetworking();

//class CAMSNETLIB Socket
//{
//public:
//	void SetBlocking();
//	void SetNonBlocking();
//
//	void Disconnect();
//
//	bool IsConnected() const;
//
//	static void GetLocalIP(LIB_TCHAR* dest);
//	static void HostNameToIP(const LIB_TCHAR* host, LIB_TCHAR* dest, UINT buffSize);
//};


class CAMSNETLIB Socket
{
public:
	Socket();
	Socket(const Socket& pc);
	Socket(Socket&& pc);
	Socket(const LIB_TCHAR* port, bool ipv6 = false);
	Socket(SOCKET pc);
	//~Socket();

	struct Hash
	{
		UINT operator()(const Socket& sock) const
		{
			return std::hash<SOCKET>()(sock.pc);
		}
	};

	Socket& operator= (const Socket& pc);
	Socket& operator= (Socket&& pc);
	bool operator== (const Socket pc) const;
	bool operator!= (const Socket pc) const;
	bool operator== (const SOCKET pc) const;
	bool operator!= (const SOCKET pc) const;

	void SetSocket(SOCKET pc);
	SOCKET GetSocket() const;

	bool Bind(const LIB_TCHAR* port, bool ipv6 = false);
	Socket AcceptConnection();

	bool Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, float timeout);

	void Shutdown();
	void Disconnect();

	long RecvData(void* dest, DWORD nBytes);
	long SendData(const void* data, DWORD nBytes);

	bool AcceptOl(SOCKET acceptSocket, void* infoBuffer, DWORD localAddrLen, DWORD remoteAddrLen, OVERLAPPED* ol);
	long RecvDataOl(WSABUF* buffer, OVERLAPPED* ol, DWORD flags = 0, UINT bufferCount = 1, LPWSAOVERLAPPED_COMPLETION_ROUTINE cr = NULL);
	long SendDataOl(WSABUF* buffer, OVERLAPPED* ol, DWORD flags = 0, UINT bufferCount = 1, LPWSAOVERLAPPED_COMPLETION_ROUTINE cr = NULL);

	bool IsConnected() const;

	bool SetTCPRecvStack(int size = 0);
	bool SetTCPSendStack(int size = 0);
	bool SetNoDelay(bool b = true);

	bool SetBlocking();
	bool SetNonBlocking();

	void SetAddrInfo(sockaddr* addr, bool cleanup);
	const SocketInfo& GetInfo();

	static int GetHostName(LIB_TCHAR* dest, DWORD nameLen);
	static char* Inet_ntot(in_addr inaddr, LIB_TCHAR* dest);
	static std::tstring GetLocalIP(bool ipv6 = false);
	static std::tstring HostNameToIP(const LIB_TCHAR* host, bool ipv6 = false);
	static void GetAcceptExAddrs(void* buffer, DWORD localAddrLen, DWORD remoteAddrLen, sockaddr** local, int* localLen, sockaddr** remote, int* remoteLen);
private:
	SOCKET pc = INVALID_SOCKET;
	SocketInfo info;
};
