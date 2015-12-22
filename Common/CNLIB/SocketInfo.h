#pragma once
#include "Typedefs.h"
#include <ws2tcpip.h>

class CAMSNETLIB SocketInfo
{
public:
	union SockaddrU
	{
		SockaddrU(sockaddr_in6* addr)
			:
			inaddr6(addr)
		{}
		sockaddr* addr;
		sockaddr_in* inaddr4;
		sockaddr_in6* inaddr6;
	};
	SocketInfo() = default;
	//addr always alloc as sockaddr_in6
	SocketInfo(SocketInfo&& socketInfo);

	void SetAddr(sockaddr* addr, size_t len);
	void Cleanup();

	SocketInfo& operator=(const SocketInfo& rhs);
	SocketInfo& operator=(SocketInfo&& rhs);

	std::tstring GetPortStr() const;
	USHORT GetPortInt() const;

	std::tstring GetIp() const;
	//Use CompareIp if you dont need ip in str format, it is quicker to compare
	bool CompareIp(const SocketInfo& rhs) const;

	bool IsIpv4() const;
	bool IsIpv6() const;

	//Can be casted to sockaddr_in or sockaddr_in6, depending on its Ipv
	SockaddrU GetSockAddr() const;
private:
	SockaddrU addr = nullptr;
};