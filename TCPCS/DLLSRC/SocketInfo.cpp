#include "StdAfx.h"
#include "CNLIB/SocketInfo.h"
#include "CNLIB/HeapAlloc.h"

SocketInfo::SocketInfo()
	:
	addr(),
	cleanup(false)
{}

SocketInfo::SocketInfo(const SocketInfo& socketInfo)
{
	*this = socketInfo;
}
SocketInfo::SocketInfo(SocketInfo&& socketInfo)
{
	*this = std::move(socketInfo);
}

void SocketInfo::Cleanup()
{
	if (cleanup)
		dealloc(addr.addr);
}

SocketInfo& SocketInfo::operator=(const SocketInfo& rhs)
{
	if (this != &rhs)
	{
		addr = rhs.addr;
		cleanup = rhs.cleanup;
	}
	return *this;
}
SocketInfo& SocketInfo::operator=(SocketInfo&& rhs)
{
	if (this != &rhs)
	{
		addr = rhs.addr;
		cleanup = rhs.cleanup;
		memset(&rhs, 0, sizeof(SocketInfo));
	}
	return *this;
}

void SocketInfo::SetAddr(sockaddr* addr, bool dealloc)
{
	cleanup = dealloc;
	if (!this->addr.addr)
		this->addr.addr = addr;
}

std::tstring SocketInfo::GetPortStr() const
{
	return To_String(GetPortInt());
}
USHORT SocketInfo::GetPortInt() const
{
	if (IsIpv4())
		return htons(addr.inaddr4->sin_port);
	else
		return htons(addr.inaddr6->sin6_port);
}

std::tstring SocketInfo::FormatIP(SockaddrU addr)
{
	if (addr.addr->sa_family == AF_INET)
	{
		std::tstringstream stream;
		sockaddr_in& sin4 = *addr.inaddr4;
		auto& data = sin4.sin_addr.S_un.S_un_b;
		stream << data.s_b1 << _T(".") << data.s_b2 << _T(".") << data.s_b3 << _T(".") << data.s_b4;
		return stream.str();
	}
	else
	{
		sockaddr_in6& sin6 = *addr.inaddr6;
		std::tstring fmt;
		int f = -1, l = 0;
		bool again = false;
		for (int i = 0; i < 8; i++)
		{
			LIB_TCHAR sw[5];
			USHORT word = ntohs(sin6.sin6_addr.s6_words[i]);
			fmt += _itot(word, sw, 16);
			if (i != 7)fmt += _T(':');
		}
		do
		{
			f = fmt.find_first_of(_T('0'), l), l = fmt.find_first_not_of(_T(":0"), f);
			if (f > 0 && (fmt[f - 1] != _T('0') && fmt[f - 1] != _T(':')))
				l = f + 2;
		} while ((f != -1 && l != -1) && (l - f) == 2);
		if (f != -1)
			fmt.erase(f, l != -1 ? l - f - 1 : l);
		if (f == 0)fmt.insert(0, _T(":"));
		if (l == -1)fmt.append(_T(":"));
		return fmt;
	}
}

std::tstring SocketInfo::GetIp() const
{
	return FormatIP(addr);
}
bool SocketInfo::CompareIp(const SocketInfo& rhs) const
{
	if (IsIpv4())
		return (addr.inaddr4->sin_addr.s_addr == addr.inaddr4->sin_addr.s_addr);
	else
		return (memcmp(addr.inaddr6->sin6_addr.u.Word, addr.inaddr6->sin6_addr.u.Word, 8 * sizeof(USHORT)) == 0);
}

bool SocketInfo::IsIpv4() const
{
	return addr.addr->sa_family == AF_INET;
}
bool SocketInfo::IsIpv6() const
{
	return addr.addr->sa_family == AF_INET6;
}

SocketInfo::SockaddrU SocketInfo::GetSockAddr() const
{
	return addr;
}
