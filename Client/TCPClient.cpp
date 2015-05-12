#include "TCPClient.h"
#include "HeapAlloc.h"
#include "Messages.h"
#include "File.h"

struct SendInfo
{
	SendInfo(TCPClient& client, char* data, const DWORD nBytes)
		:
		client(client),
		data(data),
		nBytes(nBytes)
	{}

	SendInfo(SendInfo&& info)
		:
		client(info.client),
		data(info.data),
		nBytes(info.nBytes)
	{
		ZeroMemory(&info, sizeof(SendInfo));
	}

	TCPClient& client;
	char* data;
	const DWORD nBytes;
};


TCPClient::TCPClient(cfunc func, void(*const disconFunc)(), int compression, void* obj)
	:
	function(func),
	disconFunc(disconFunc),
	obj(obj),
	compression(compression),
	host(INVALID_SOCKET),
	recv(NULL)
{}

TCPClient::TCPClient(TCPClient&& client)
	:
	host(client.host),
	function(client.function),
	disconFunc(client.disconFunc),
	obj(client.obj),
	recv(client.recv),
	compression(client.compression)
{
	ZeroMemory(&client, sizeof(TCPClient));
}

TCPClient& TCPClient::operator=(TCPClient&& client)
{
	if(this != &client)
	{
		this->~TCPClient();

		host = client.host;
		function = client.function;
		const_cast<void(*&)()>(disconFunc) = client.disconFunc;
		obj = client.obj;
		recv = client.recv;
		sendSect = client.sendSect;
		const_cast<int&>(compression) = client.compression;

		ZeroMemory(&client, sizeof(TCPClient));
	}
	return *this;
}

TCPClient::~TCPClient()
{
	Disconnect();
}

static DWORD CALLBACK SendData(LPVOID param)
{
	SendInfo* data = (SendInfo*)param;
	TCPClient& client = data->client;
	Socket& pc = client.GetHost();
	CRITICAL_SECTION* sendSect = client.GetSendSect();

	EnterCriticalSection(sendSect);

	if (pc.SendData(&data->nBytes, sizeof(DWORD)) > 0)
	{
		DWORD nBytesComp = FileMisc::GetCompressedBufferSize(data->nBytes);
		BYTE* dataComp = alloc<BYTE>(nBytesComp);
		nBytesComp = FileMisc::Compress(dataComp, nBytesComp, (const BYTE*)data->data, data->nBytes, client.GetCompression());
		if (pc.SendData(&nBytesComp, sizeof(DWORD)) > 0)
		{
			if (pc.SendData(dataComp, nBytesComp) > 0)
			{
				dealloc(dataComp);
				destruct(data);
				LeaveCriticalSection(sendSect);
				return 0;
			}
			else dealloc(dataComp);
		}
		else dealloc(dataComp);
	}

	destruct(data);
	client.Disconnect();
	LeaveCriticalSection(sendSect);
	return 0;
}

static DWORD CALLBACK ReceiveData(LPVOID param)
{
	TCPClient& client = *(TCPClient*)param;
	DWORD nBytesComp = 0, nBytesDecomp = 0;
	Socket& host = client.GetHost();

	while (host.IsConnected())
	{
		if (host.ReadData(&nBytesDecomp, sizeof(DWORD)) > 0)
		{
			if (host.ReadData(&nBytesComp, sizeof(DWORD)) > 0)
			{
				BYTE* compBuffer = alloc<BYTE>(nBytesComp);
				if (host.ReadData(compBuffer, nBytesComp) > 0)
				{
					BYTE* deCompBuffer = alloc<BYTE>(nBytesDecomp);
					FileMisc::Decompress(deCompBuffer, nBytesDecomp, compBuffer, nBytesComp);
					dealloc(compBuffer);
					(*client.GetFunction())(param, deCompBuffer, nBytesDecomp, client.GetObj());
					dealloc(deCompBuffer);
				}
				else
				{
					dealloc(compBuffer);
					break;
				}
			}
			else break;
		}
		else break;
	}

	client.GetDisfunc()();
	client.Disconnect();

	return 0;
}

//IP or HostName for dest
bool TCPClient::Connect( const TCHAR* dest, const TCHAR* port, float timeOut )
{
	if(host.IsConnected())
		return false;

	host.Connect( dest, port, timeOut );

	if(!host.IsConnected())
		return false;

	return true;
}

void TCPClient::Disconnect()
{
	host.Disconnect(); //causes recvThread to close
	if (recv)
	{
		CloseHandle(recv);
		recv = NULL;
		DeleteCriticalSection(&sendSect);
	}
}

HANDLE TCPClient::SendServData(const char* data, DWORD nBytes)
{
	return CreateThread(NULL, 0, SendData, (LPVOID)construct<SendInfo>(SendInfo(*this, (char*)data, nBytes)), NULL, NULL);
}

bool TCPClient::RecvServData()
{
	if(!host.IsConnected())
		return false;

	recv = CreateThread(NULL, 0, ReceiveData, this, NULL, NULL);
	InitializeCriticalSection(&sendSect);

	return true;
}

void TCPClient::SendMsg(char type, char message)
{
	char msg[] = { type, message };

	HANDLE hnd = SendServData(msg, MSG_OFFSET);
	TCPClient::WaitAndCloseHandle(hnd);
}

void TCPClient::SendMsg(std::tstring& name, char type, char message)
{
	const DWORD nBytes = MSG_OFFSET + ((name.size() + 1) * sizeof(TCHAR));
	char* msg = alloc<char>(nBytes);
	msg[0] = type;
	msg[1] = message;
	memcpy(&msg[MSG_OFFSET], name.c_str(), nBytes - MSG_OFFSET);
	HANDLE hnd = SendServData(msg, nBytes);
	TCPClient::WaitAndCloseHandle(hnd);
	dealloc(msg);
}

void TCPClient::WaitAndCloseHandle(HANDLE& hnd)
{
	WaitForSingleObject(hnd, INFINITE);
	CloseHandle(hnd);
	hnd = NULL; //NULL instead of INVALID_HANDLE_VALUE due to move ctor
}

void TCPClient::Ping()
{
	SendMsg(TYPE_PING, MSG_PING);
}

void TCPClient::SetFunction(cfunc function)
{
	this->function = function;
}

cfuncP TCPClient::GetFunction()
{
	return &function;
}

Socket& TCPClient::GetHost()
{
	return host;
}

void* TCPClient::GetObj() const
{
	return obj;
}

void TCPClient::WaitForRecvThread() const
{
	WaitForSingleObject(recv, INFINITE);
}

int TCPClient::GetCompression() const
{
	return compression;
}

CRITICAL_SECTION* TCPClient::GetSendSect()
{
	return &sendSect;
}

void(*TCPClient::GetDisfunc()) ()
{
	return disconFunc;
}

bool TCPClient::IsConnected() const
{
	return host.IsConnected();
}
