#include "TCPServ.h"
#include "HeapAlloc.h"
#include "File.h"
#include "Messages.h"
#include "MsgStream.h"

TCPServInterface* CreateServer(sfunc msgHandler, customFunc conFunc, customFunc disFunc, USHORT maxCon, int compression, float pingInterval, void* obj)
{
	return construct<TCPServ>(msgHandler, conFunc, disFunc, maxCon, compression, pingInterval, obj);
}

void DestroyServer(TCPServInterface*& server)
{
	destruct((TCPServ*&)server);
}

struct Data
{
	Data(TCPServ& serv, USHORT pos)
		:
		serv(serv),
		pos(pos)
	{}
	Data(Data&& data)
		:
		serv(data.serv),
		pos(data.pos)
	{
		ZeroMemory(&data, sizeof(Data));
	}

	TCPServ& serv;
	USHORT pos;
};

struct SAData
{
	SAData(TCPServ& serv, const char* data, DWORD nBytesDecomp, Socket exAddr, const bool single, CompressionType compType)
		:
		serv(serv),
		data(data),
		nBytesDecomp(nBytesDecomp),
		exAddr(exAddr),
		single(single),
		compType(compType)
	{}
	SAData(SAData&& data)
		:
		serv(data.serv),
		data(data.data),
		nBytesDecomp(data.nBytesDecomp),
		exAddr(data.exAddr),
		single(data.single),
		compType(data.compType)
	{
		ZeroMemory(&data, sizeof(SAData));
	}

	TCPServ& serv;
	DWORD nBytesDecomp;
	const char* data;
	Socket exAddr;
	bool single;
	CompressionType compType;
};

struct SADataEx
{
	SADataEx(TCPServ& serv, const char* data, DWORD nBytesDecomp, Socket* pcs, USHORT nPcs, CompressionType compType)
		:
		serv(serv),
		data(data),
		nBytesDecomp(nBytesDecomp),
		pcs(pcs),
		nPcs(nPcs),
		compType(compType)
	{}
	SADataEx(SADataEx&& data)
		:
		serv(data.serv),
		data(data.data),
		nBytesDecomp(data.nBytesDecomp),
		pcs(data.pcs),
		nPcs(data.nPcs),
		compType(data.compType)
	{
		ZeroMemory(&data, sizeof(SADataEx));
	}

	TCPServ& serv;
	DWORD nBytesDecomp;
	const char* data;
	Socket* pcs;
	USHORT nPcs;
	CompressionType compType;
};

TCPServ::ClientData::ClientData(TCPServ& serv, Socket pc, sfunc func, USHORT recvIndex)
	:
	pc(pc),
	func(func),
	recvIndex(recvIndex),
	recvThread(INVALID_HANDLE_VALUE),
	obj(nullptr)
{}

TCPServ::ClientData::ClientData(ClientData&& clint)
	:
	pc(std::move(clint.pc)),
	func(clint.func),
	user(std::move(clint.user)),
	recvIndex(clint.recvIndex),
	recvThread(clint.recvThread),
	obj(clint.obj)
{
	ZeroMemory(&clint, sizeof(ClientData));
}

TCPServ::ClientData& TCPServ::ClientData::operator=(ClientData&& data)
{
	if(this != &data)
	{
		pc = std::move(data.pc);
		func = data.func;
		user = std::move(data.user);
		recvIndex = data.recvIndex;
		recvThread = data.recvThread;
		obj = data.obj;

		ZeroMemory(&data, sizeof(ClientData));
	}
	return *this;
}


static void SendDataComp(Socket pc, const char* data, DWORD nBytesDecomp, DWORD nBytesComp)
{
	const DWORD64 nBytes = ((DWORD64)nBytesDecomp) << 32 | nBytesComp;

	//No need to remove client, that is handled in recv thread
	if (pc.SendData(&nBytes, sizeof(DWORD64)) > 0)
		pc.SendData(data, (nBytesComp != 0) ? nBytesComp : nBytesDecomp);
}

static DWORD CALLBACK SendAllData(LPVOID info)
{
	SAData* data = (SAData*)info;
	TCPServ& serv = data->serv;
	auto clients = serv.GetClients();
	CRITICAL_SECTION* sect = serv.GetSendSect();
	Socket exAddr = data->exAddr;
	BYTE* dataComp = (BYTE*)data->data;
	const DWORD nBytesDecomp = data->nBytesDecomp;
	DWORD nBytesComp = 0;

	if (data->compType == SETCOMPRESSION)
	{
		const BYTE* dataDeComp = dataComp;
		nBytesComp = FileMisc::GetCompressedBufferSize(nBytesDecomp);
		dataComp = alloc<BYTE>(nBytesComp);
		nBytesComp = FileMisc::Compress(dataComp, nBytesComp, dataDeComp, nBytesDecomp, serv.GetCompression());
		data->data = (char*)dataComp;
	}

	EnterCriticalSection(sect);

	if (data->single)
	{
		if (exAddr.IsConnected())
			SendDataComp(exAddr, (const char*)dataComp, nBytesDecomp, nBytesComp);
	}

	else
	{
		const USHORT nClients = serv.ClientCount();
		if (data->exAddr.IsConnected())
		{
			for (USHORT i = 0; i < nClients; i++)
				if (clients[i]->pc != exAddr)
					SendDataComp(clients[i]->pc, (const char*)dataComp, nBytesDecomp, nBytesComp);
		}
		else
		{
			for (USHORT i = 0; i < nClients; i++)
				SendDataComp(clients[i]->pc, (const char*)dataComp, nBytesDecomp, nBytesComp);
		}
	}

	LeaveCriticalSection(sect);

	dealloc(dataComp);
	destruct(data);

	return 0;
}

static DWORD CALLBACK SendAllDataEx(LPVOID info)
{
	SADataEx* data = (SADataEx*)info;
	TCPServ& serv = data->serv;
	Socket* pcs = data->pcs;
	const USHORT pcCount = data->nPcs;
	CRITICAL_SECTION* sect = serv.GetSendSect();
	BYTE* dataComp = (BYTE*)data->data;
	const DWORD nBytesDecomp = data->nBytesDecomp;
	DWORD nBytesComp = 0;

	if (data->compType == SETCOMPRESSION)
	{
		const BYTE* dataDeComp = dataComp;
		nBytesComp = FileMisc::GetCompressedBufferSize(nBytesDecomp);
		dataComp = alloc<BYTE>(nBytesComp);
		nBytesComp = FileMisc::Compress(dataComp, nBytesComp, dataDeComp, nBytesDecomp, serv.GetCompression());
		data->data = (char*)dataComp;
	}

	EnterCriticalSection(sect);

	for (USHORT i = 0; i < pcCount; i++)
	{
		if (pcs[i].IsConnected())
			SendDataComp(pcs[i], (const char*)dataComp, nBytesDecomp, nBytesComp);
	}
	LeaveCriticalSection(sect);

	dealloc(dataComp);
	destruct(data);

	return 0;
}

DWORD CALLBACK WaitForConnections(LPVOID tcpServ)
{
	TCPServ& serv = *(TCPServ*)tcpServ;
	Socket& host = serv.GetHost();

	while( host.IsConnected() )
	{
		Socket temp = host.AcceptConnection();
		if( temp.IsConnected() )
		{
			if(!serv.MaxClients())
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
	return 0;
}

static DWORD CALLBACK RecvData(LPVOID info)
{
	Data* data = (Data*)info;
	TCPServ& serv = data->serv;
	TCPServ::ClientData** clients = serv.GetClients();
	USHORT& pos = clients[data->pos]->recvIndex;
	TCPServ::ClientData* clint = clients[pos];
	Socket& pc = clint->pc;
	void* obj = serv.GetObj();
	DWORD64 nBytes = 0;

	while(pc.IsConnected())
	{
		if(pc.ReadData(&nBytes, sizeof(DWORD64)) > 0)
		{
			const DWORD nBytesDecomp = nBytes >> 32;
			const DWORD nBytesComp = nBytes & 0xffffffff;
			BYTE* buffer = alloc<BYTE>(nBytesDecomp + nBytesComp);
			nBytes = (nBytesComp != 0) ? nBytesComp : nBytesDecomp;

			if (pc.ReadData(buffer, nBytes) > 0)
			{
				BYTE* dest = &buffer[nBytesComp];
				if (nBytesComp != 0)
					FileMisc::Decompress(dest, nBytesDecomp, buffer, nBytesComp);

				(clint->func)(serv, clint, dest, nBytes, obj);
				dealloc(buffer);
			}
			else
			{
				dealloc(buffer);
				break;
			}
		}
		else break;
	}

	serv.RemoveClient(pos);
	destruct(data);
	return 0;
}


void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, CompressionType compType)
{
	if (compType == BESTFIT)
	{
		if (nBytes >= 128)
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}

	SendAllData(construct<SAData>(*this, data, nBytes, exAddr, single, compType));
}

void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, CompressionType compType)
{
	if (compType == BESTFIT)
	{
		if (nBytes >= 128)
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}
	SendAllDataEx(construct<SADataEx>(*this, data, nBytes, pcs, nPcs, compType));
}

void TCPServ::SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType)
{
	SendClientData(data, nBytes, pcs.data(), pcs.size(), compType);
}

HANDLE TCPServ::SendClientDataThread(const char* data, DWORD nBytes, Socket exAddr, bool single, CompressionType compType)
{
	if (compType == BESTFIT)
	{
		if (nBytes >= 128)
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}
	return CreateThread(NULL, 0, &SendAllData, (LPVOID)construct<SAData>(*this, data, nBytes, exAddr, single, compType), NULL, NULL);
}

HANDLE TCPServ::SendClientDataThread(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, CompressionType compType)
{
	if (compType == BESTFIT)
	{
		if (nBytes >= 128)
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}
	return CreateThread(NULL, 0, &SendAllDataEx, (LPVOID)construct<SADataEx>(*this, data, nBytes, pcs, nPcs, compType), NULL, NULL);
}

HANDLE TCPServ::SendClientDataThread(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType)
{
	return SendClientDataThread(data, nBytes, pcs.data(), pcs.size(), compType);
}

void TCPServ::SendMsg(Socket pc, bool single, char type, char message)
{
	char msg[] = { type, message };

	SendClientData(msg, MSG_OFFSET, pc, single, NOCOMPRESSION);
}

void TCPServ::SendMsg(Socket* pcs, USHORT nPcs, char type, char message)
{
	char msg[] = { type, message };

	SendClientData(msg, MSG_OFFSET, pcs, nPcs, NOCOMPRESSION);
}

void TCPServ::SendMsg(std::vector<Socket>& pcs, char type, char message)
{
	SendMsg(pcs.data(), pcs.size(), type, message);
}

void TCPServ::SendMsg(const std::tstring& user, char type, char message)
{
	SendMsg(FindClient(user)->pc, true, type, message);
}


TCPServ::TCPServ(sfunc func, customFunc conFunc, customFunc disFunc, USHORT maxCon, int compression, float pingInterval, void* obj)
	:
	host(INVALID_SOCKET),
	clients(nullptr),
	nClients(0),
	function(func),
	obj(obj),
	conFunc(conFunc),
	disFunc(disFunc),
	openCon(NULL),
	compression(compression),
	maxCon(maxCon),
	pingHandler(*this)
{
	pingHandler.SetPingTimer(pingInterval);
}

TCPServ::TCPServ(TCPServ&& serv)
	:
	host(serv.host),
	clients(serv.clients), 
	nClients(serv.nClients),
	function(serv.function),
	obj(serv.obj),
	conFunc(serv.conFunc),
	disFunc(serv.disFunc),
	clientSect(serv.clientSect),
	sendSect(serv.sendSect),
	openCon(serv.openCon),
	compression(serv.compression),
	maxCon(serv.maxCon),
	pingHandler(serv.pingHandler)
{
	ZeroMemory(&serv, sizeof(TCPServ));
}

TCPServ& TCPServ::operator=(TCPServ&& serv)
{
	if(this != &serv)
	{
		this->~TCPServ();

		host = serv.host;
		clients = serv.clients;
		nClients = serv.nClients;
		function = serv.function;
		obj = serv.obj;
		const_cast<void( *& )(ClientData*)>(conFunc) = serv.conFunc;
		const_cast<void( *& )(ClientData*)>(disFunc) = serv.disFunc;
		clientSect = serv.clientSect;
		sendSect = serv.sendSect;
		openCon = serv.openCon;
		const_cast<int&>(compression) = serv.compression;
		const_cast<USHORT&>(maxCon) = serv.maxCon;
		pingHandler = std::move(serv.pingHandler);

		ZeroMemory(&serv, sizeof(TCPServ));
	}
	return *this;
}

TCPServ::~TCPServ()
{
	Shutdown();
}

bool TCPServ::AllowConnections(const LIB_TCHAR* port)
{
	if(host.IsConnected() || !host.Bind(port))
		return false;

	if(!clients)
		clients = alloc<ClientData*>(maxCon);

	openCon = CreateThread(NULL, 0, WaitForConnections, this, NULL, NULL);

	if(!openCon)
		return false;

	InitializeCriticalSection(&clientSect);
	InitializeCriticalSection(&sendSect);

	return true;
}

void TCPServ::AddClient(Socket pc)
{
	EnterCriticalSection(&clientSect);

	Data* data = construct<Data>(*this, nClients);
	clients[nClients] = construct<ClientData>(data->serv, pc, function, data->pos);
	clients[nClients]->recvThread = CreateThread(NULL, 0, RecvData, data, NULL, NULL);

	RunConFunc(clients[nClients]);

	++nClients;

	LeaveCriticalSection(&clientSect);
}

void TCPServ::RemoveClient(USHORT& pos)
{
	EnterCriticalSection(&clientSect);

	const USHORT index = pos;

	ClientData& data = *clients[index];

	//If user wasn't removed intentionaly
	if (clients[index]->pc.IsConnected())
		RunDisFunc(clients[index]);

	std::tstring user = std::move(data.user);

	data.pc.Disconnect();
	CloseHandle(data.recvThread);
	destruct(clients[index]);

	if(index != (nClients - 1))
	{
		clients[index] = clients[nClients - 1];
		clients[index]->recvIndex = index;
	}

	--nClients;

	LeaveCriticalSection(&clientSect);

	if(!user.empty() && host.IsConnected())//if user wasnt declined authentication
	{
		MsgStreamWriter streamWriter(TYPE_CHANGE, MSG_CHANGE_DISCONNECT, (user.size() + 1) * sizeof(LIB_TCHAR));
		streamWriter.WriteEnd(user.c_str());
		SendClientData(streamWriter, streamWriter.GetSize(), Socket(), false);
	}
}

TCPServ::ClientData* TCPServ::FindClient(const std::tstring& user) const
{
	for (USHORT i = 0; i < nClients; i++)
	{
		if (clients[i]->user.compare(user) == 0)
			return clients[i];
	}
	return nullptr;
}

void TCPServ::DisconnectClient(ClientData* client)
{
	RunDisFunc(client);
	client->pc.Disconnect();
}

void TCPServ::Shutdown()
{
	if (openCon)
	{
		host.Disconnect();//causes opencon thread to close
		WaitAndCloseHandle(openCon);

		//nClients != 0 because nClients changes after thread ends
		//close recv threads and free memory
		while(nClients != 0)
		{
			ClientData& data = *clients[nClients - 1];
			data.pc.Disconnect();
			WaitForSingleObject(data.recvThread, INFINITE); //handle closed in RemoveClient
		}

		dealloc(clients);

		DeleteCriticalSection(&clientSect);
		DeleteCriticalSection(&sendSect);
	}
}

void TCPServ::Ping(Socket client)
{
	SendMsg(client, true, TYPE_PING, MSG_PING);
}

void TCPServ::PingAll()
{
	SendMsg(Socket(), false, TYPE_PING, MSG_PING);
}

Socket& TCPServ::GetHost()
{
	return host;
}

bool TCPServ::MaxClients() const
{
	return nClients == maxCon;
}

TCPServ::ClientData** TCPServ::GetClients() const
{
	return clients;
}

USHORT TCPServ::ClientCount() const
{
	return nClients;
}

void TCPServ::SetPingInterval(float interval)
{
	pingHandler.SetPingTimer(interval);
}

void TCPServ::RunConFunc(ClientData* client)
{
	conFunc(client);
}

void TCPServ::RunDisFunc(ClientData* client)
{
	disFunc(client);
}

void* TCPServ::GetObj() const
{
	return obj;
}

int TCPServ::GetCompression() const
{
	return compression;
}

bool TCPServ::IsConnected() const
{
	return host.IsConnected();
}

CRITICAL_SECTION* TCPServ::GetSendSect()
{
	return &sendSect;
}

