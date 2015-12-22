#include "TCPClient.h"
#include "HeapAlloc.h"
#include "Messages.h"
#include "File.h"
#include "MsgStream.h"

TCPClientInterface* CreateClient(cfunc msgHandler, dcfunc disconFunc, int compression, float pingInterval, void* obj)
{
	return construct<TCPClient>(msgHandler, disconFunc, compression, pingInterval, obj);
}

void DestroyClient(TCPClientInterface*& client)
{
	destruct((TCPClient*&)client);
}



struct SendInfo
{
	SendInfo(TCPClient& client, char* data, const DWORD nBytes, CompressionType compType)
		:
		client(client),
		data(data),
		nBytes(nBytes),
		compType(compType)
	{}

	SendInfo(SendInfo&& info)
		:
		client(info.client),
		data(info.data),
		nBytes(info.nBytes),
		compType(info.compType)
	{
		ZeroMemory(&info, sizeof(SendInfo));
	}

	TCPClient& client;
	char* data;
	const DWORD nBytes;
	CompressionType compType;
};

TCPClient::TCPClient(cfunc func, dcfunc disconFunc, int compression, float pingInterval, void* obj)
	:
	host(INVALID_SOCKET),
	function(func),
	disconFunc(disconFunc),
	obj(obj),
	recv(NULL),
	compression(compression),
	unexpectedShutdown(true),
	pingInterval(pingInterval),
	pingHandler(nullptr)
{}

TCPClient::TCPClient(TCPClient&& client)
	:
	host(client.host),
	function(client.function),
	disconFunc(client.disconFunc),
	obj(client.obj),
	recv(client.recv),
	sendSect(client.sendSect),
	compression(client.compression),
	unexpectedShutdown(client.unexpectedShutdown),
	pingInterval(client.pingInterval),
	pingHandler(client.pingHandler)
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
		const_cast<std::remove_const_t<dcfunc>&>(disconFunc) = client.disconFunc;
		obj = client.obj;
		recv = client.recv;
		sendSect = client.sendSect;
		const_cast<int&>(compression) = client.compression;
		unexpectedShutdown = client.unexpectedShutdown;
		pingInterval = client.pingInterval;
		pingHandler = client.pingHandler;

		ZeroMemory(&client, sizeof(TCPClient));
	}
	return *this;
}

TCPClient::~TCPClient()
{
	Shutdown();
}

void TCPClient::Cleanup()
{
	destruct(pingHandler);
	host.Disconnect();
	DeleteCriticalSection(&sendSect);
	if (recv)
	{
		CloseHandle(recv);
		recv = NULL;
	}
}

static void SendDataComp(Socket pc, const char* data, DWORD nBytesDecomp, DWORD nBytesComp)
{
	const DWORD64 nBytes = ((DWORD64)nBytesDecomp) << 32 | nBytesComp;

	//No need to remove client, that is handled in recv thread
	if (pc.SendData(&nBytes, sizeof(DWORD64)) > 0)
		pc.SendData(data, (nBytesComp != 0) ? nBytesComp : nBytesDecomp);
}

static DWORD CALLBACK SendData(LPVOID param)
{
	SendInfo* data = (SendInfo*)param;
	TCPClient& client = data->client;
	Socket& pc = client.GetHost();
	CRITICAL_SECTION* sendSect = client.GetSendSect();
	const BYTE* dataDecomp = (const BYTE*)data->data;
	BYTE* dataComp = nullptr;
	DWORD nBytesComp = 0;
	DWORD nBytesDecomp = data->nBytes;

	if (data->compType == SETCOMPRESSION)
	{
		nBytesComp = FileMisc::GetCompressedBufferSize(nBytesDecomp);
		dataComp = alloc<BYTE>(nBytesComp);
		nBytesComp = FileMisc::Compress(dataComp, nBytesComp, dataDecomp, nBytesDecomp, client.GetCompression());
	}

	const char* dat = (const char*)(dataComp ? dataComp : dataDecomp);

	EnterCriticalSection(sendSect);

	SendDataComp(pc, dat, nBytesDecomp, nBytesComp);

	LeaveCriticalSection(sendSect);

	dealloc(dataComp);
	destruct(data);
	return 0;
}

static DWORD CALLBACK ReceiveData(LPVOID param)
{
	TCPClient& client = *(TCPClient*)param;
	Socket& host = client.GetHost();
	cfuncP func = client.GetFunction();
	void* obj = client.GetObj();
	DWORD64 nBytes = 0;

	while (host.IsConnected())//break out if you disconnected from server
	{
		if (host.ReadData(&nBytes, sizeof(DWORD64)) > 0)
		{
			const DWORD nBytesDecomp = nBytes >> 32;
			const DWORD nBytesComp = nBytes & 0xffffffff;
			BYTE* buffer = alloc<BYTE>(nBytesDecomp + nBytesComp);

			if (host.ReadData(buffer, (nBytesComp != 0) ? nBytesComp : nBytesDecomp) > 0)
			{
				BYTE* dest = buffer + nBytesComp;
				if (nBytesComp != 0)
					FileMisc::Decompress(dest, nBytesDecomp, buffer, nBytesComp);

				(*func)(client, dest, nBytesDecomp, obj);
				dealloc(buffer);
			}
			else
			{
				dealloc(buffer);
				break;
			}
		}
		else
			break;
	}

	client.RunDisconFunc();

	//Cleanup
	client.Cleanup();

	return 0;
}

//IP or HostName for dest
bool TCPClient::Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, float timeOut)
{
	if(host.IsConnected())
		return false;

	//reset so it will send correct message
	SetShutdownReason(true);

	return host.Connect(dest, port, ipv6, timeOut);
}

void TCPClient::Disconnect()
{
	SetShutdownReason(false);
	host.Disconnect(); //causes recvThread to close
}

void TCPClient::Shutdown()
{
	if(recv)
	{
		Disconnect();
		WaitForSingleObject(recv, INFINITE);//Handle closed in thread
	}
}

void TCPClient::SendServData(const char* data, DWORD nBytes, CompressionType compType)
{
	if (compType == BESTFIT)
	{
		if (nBytes >= COMPBYTECO)
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}
	SendData(construct<SendInfo>(*this, (char*)data, nBytes, compType));
}

HANDLE TCPClient::SendServDataThread(const char* data, DWORD nBytes, CompressionType compType)
{
	if (compType == BESTFIT)
	{
		if (nBytes >= COMPBYTECO)
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}
	return CreateThread(NULL, 0, SendData, construct<SendInfo>(*this, (char*)data, nBytes, compType), NULL, NULL);
}

bool TCPClient::RecvServData()
{
	if (!host.IsConnected())
		return false;

	recv = CreateThread(NULL, 0, ReceiveData, this, NULL, NULL);

	if (!recv)
		return false;

	pingHandler = construct<PingHandler>(this);

	if (!pingHandler)
		return false;

	pingHandler->SetPingTimer(pingInterval);

	InitializeCriticalSection(&sendSect);

	return true;
}

void TCPClient::SendMsg(char type, char message)
{
	char msg[] = { type, message };

	SendServData(msg, MSG_OFFSET, NOCOMPRESSION);
}

void TCPClient::SendMsg(const std::tstring& name, char type, char message)
{
	MsgStreamWriter streamWriter(type, message, StreamWriter::SizeType(name));
	streamWriter.Write(name);
	SendServData(streamWriter, streamWriter.GetSize(), NOCOMPRESSION);
}

void TCPClient::Ping()
{
	SendMsg(TYPE_PING, MSG_PING);
}

void TCPClient::SetFunction(cfunc function)
{
	this->function = function;
}

void TCPClient::RunDisconFunc()
{
	disconFunc(unexpectedShutdown);
}

void TCPClient::SetShutdownReason(bool unexpected)
{
	unexpectedShutdown = unexpected;
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

int TCPClient::GetCompression() const
{
	return compression;
}

CRITICAL_SECTION* TCPClient::GetSendSect()
{
	return &sendSect;
}

dcfunc TCPClient::GetDisfunc() const
{
	return disconFunc;
}

bool TCPClient::IsConnected() const
{
	return host.IsConnected();
}

void TCPClient::SetPingInterval(float interval)
{
	pingInterval = interval;
	pingHandler->SetPingTimer(pingInterval);
}

float TCPClient::GetPingInterval() const
{
	return pingInterval;
}
