#include "SocketListen.h"
#include "TCPServ.h"
#include "Messages.h"

struct ConnData
{
	ConnData(SocketListen& listenSocket, TCPServ& serv)
		:
		listenSocket(listenSocket),
		serv(serv)
	{}

	SocketListen& listenSocket;
	TCPServ& serv;
};

DWORD CALLBACK WaitForConnections(LPVOID data)
{
	ConnData* connData = (ConnData*)data;
	Socket& host = connData->listenSocket.GetHost();
	TCPServ& serv = connData->serv;

	while (host.IsConnected())
	{
		Socket temp = host.AcceptConnection();
		if (temp.IsConnected())
		{
			if (!serv.MaxClients())
			{
				serv.AddClient(temp);
			}
			else
			{
				serv.SendMsg(temp, true, TYPE_CHANGE, MSG_CHANGE_SERVERFULL);
				temp.Disconnect();
			}
		}
	}

	destruct(connData);
	return 0;
}

SocketListen::SocketListen()
	:
	host(),
	openCon(NULL)
{}

SocketListen::SocketListen(SocketListen&& socket)
	:
	host(socket.host),
	openCon(socket.openCon)
{
	ZeroMemory(&socket, sizeof(SocketListen));
}

SocketListen::~SocketListen()
{
	Destroy();
}

void SocketListen::Destroy()
{
	if (openCon)
	{
		host.Disconnect();
		WaitAndCloseHandle(openCon);
	}
}

bool SocketListen::Bind(const LIB_TCHAR* port, bool ipv6)
{
	return host.Bind(port, ipv6);
}

void SocketListen::StartThread(TCPServ& serv)
{
	openCon = CreateThread(NULL, 0, WaitForConnections, construct<ConnData>(*this, serv), NULL, NULL);
}

Socket& SocketListen::GetHost()
{
	return host;
}
