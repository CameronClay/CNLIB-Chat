#pragma once
#include "Typedefs.h"
#include "HeapAlloc.h"
#include <ws2tcpip.h>

class CAMSNETLIB SocketInfo
{
public:
	union CAMSNETLIB SockaddrU
	{
		SockaddrU()
			:
			addr(nullptr)
		{}
		SockaddrU(sockaddr* addr)
			:
			addr(addr)
		{}
		sockaddr* addr;
		sockaddr_in* inaddr4;
		sockaddr_in6* inaddr6;
	};
	SocketInfo();
	SocketInfo(const SocketInfo& socketInfo);
	SocketInfo(SocketInfo&& socketInfo);

	void Cleanup();

	void SetAddr(sockaddr* addr, bool cleanup);

	SocketInfo& operator=(const SocketInfo& rhs);
	SocketInfo& operator=(SocketInfo&& rhs);

	std::tstring GetPortStr() const;
	USHORT GetPortInt() const;

	static std::tstring FormatIP(SockaddrU addr);
	std::tstring GetIp() const;
	//Use CompareIp if you dont need ip in str format, it is quicker to compare
	bool CompareIp(const SocketInfo& rhs) const;

	bool IsIpv4() const;
	bool IsIpv6() const;

	//Can be casted to sockaddr_in or sockaddr_in6, depending on its Ipv
	SockaddrU GetSockAddr() const;
private:
	SockaddrU addr;
	bool cleanup;
};
