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
	Socket(const LIB_TCHAR* port);
	Socket(SOCKET pc);

	struct Hash
	{
		size_t operator()(const Socket& sock) const
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

	operator SOCKET&();
	operator HANDLE&();

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

	//dest size should be INET6_ADDRSTRLEN
	static bool GetLocalIP(LIB_TCHAR* dest, bool ipv6 = false);
	static bool HostNameToIP(const LIB_TCHAR* host, LIB_TCHAR* dest, DWORD buffSize, bool ipv6 = false);

private:
	SOCKET pc;
	SocketInfo info;
};
