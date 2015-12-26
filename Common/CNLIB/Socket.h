//Copyright (c) <2015> <Cameron Clay>

#pragma once

#include "Typedefs.h"
#include <tchar.h>
#include "IPv.h"
#include "SocketInfo.h"

#if NTDDI_VERSION >= NTDDI_VISTA
#include <Natupnp.h>
#pragma comment(lib,"Ntdll.lib")
#endif

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
	~Socket();

	struct Hash
	{
		size_t operator()(const Socket& sock) const
		{
			return std::hash<SOCKET>()(*sock.pc);
		}
	};

	Socket& operator= (const Socket& pc);
	Socket& operator= (Socket&& pc);
	bool operator== (const Socket pc) const;
	bool operator!= (const Socket pc) const;
	bool operator== (const SOCKET pc) const;
	bool operator!= (const SOCKET pc) const;

	void SetSocket(SOCKET pc);

	bool Bind(const LIB_TCHAR* port, bool ipv6 = false);
	Socket AcceptConnection();

	bool Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, float timeout);
	void Disconnect();

	long ReadData(void* dest, DWORD nBytes);
	long SendData(const void* data, DWORD nBytes);

	bool IsConnected() const;

	bool SetBlocking();
	bool SetNonBlocking();

	const SocketInfo& GetInfo();

	//dest size should be INET6_ADDRSTRLEN for ipv6, or INET_ADDRSTRLEN for ipv4
	static bool GetLocalIP(LIB_TCHAR* dest, bool ipv6 = false);
	static bool HostNameToIP(const LIB_TCHAR* host, LIB_TCHAR* dest, bool ipv6 = false);

	UINT GetRefCount() const;
private:
	SOCKET* pc = nullptr;
	UINT* refCount = nullptr;
	SocketInfo info;
};
