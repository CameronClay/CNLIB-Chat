#include "TCPServ.h"
#include "HeapAlloc.h"
#include "File.h"
#include "Messages.h"
#include "Format.h"
#include "MsgStream.h"

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
	SAData(TCPServ& serv, char* data, DWORD nBytesDecomp, Socket exAddr, const bool single)
		:
		serv(serv),
		nBytesComp(0),
		data(data),
		nBytesDecomp(nBytesDecomp),
		exAddr(exAddr),
		single(single)
	{}
	SAData(SAData&& data)
		:
		serv(data.serv),
		nBytesComp(data.nBytesComp),
		data(data.data),
		nBytesDecomp(data.nBytesDecomp),
		exAddr(data.exAddr),
		single(data.single)
	{
		ZeroMemory(&data, sizeof(SAData));
	}

	TCPServ& serv;
	DWORD nBytesComp, nBytesDecomp;
	char* data;
	Socket exAddr;
	bool single;
};

struct SADataEx
{
	SADataEx(TCPServ& serv, char* data, DWORD nBytesDecomp, Socket* pcs, USHORT nPcs)
		:
		serv(serv),
		nBytesComp(0),
		data(data),
		nBytesDecomp(nBytesDecomp),
		pcs(pcs),
		nPcs(nPcs)
	{}
	SADataEx(SADataEx&& data)
		:
		serv(data.serv),
		nBytesComp(data.nBytesComp),
		data(data.data),
		nBytesDecomp(data.nBytesDecomp),
		pcs(data.pcs),
		nPcs(data.nPcs)
	{
		ZeroMemory(&data, sizeof(SADataEx));
	}

	TCPServ& serv;
	DWORD nBytesComp, nBytesDecomp;
	char* data;
	Socket* pcs;
	USHORT nPcs;
};

struct SData
{
	SData(SAData* saData, Socket pc)
		:
		saData(saData),
		pc(pc)
	{}
	SData(SData&& data)
		:
		saData(data.saData),
		pc(data.pc)
	{
		ZeroMemory(&data, sizeof(SData));
	}
	SAData* const saData;
	Socket pc;
};

struct SDataEx
{
	SDataEx(SADataEx* saData, Socket pc)
		:
		saData(saData),
		pc(pc)
	{}
	SDataEx(SDataEx&& data)
		:
		saData(data.saData),
		pc(data.pc)
	{
		ZeroMemory(&data, sizeof(SDataEx));
	}
	SADataEx* const saData;
	Socket pc;
};


TCPServ::ClientData::ClientData(TCPServ& serv, Socket pc, sfunc func, USHORT recvIndex)
	:
	pc(pc),
	func(func),
	pingHandler(serv, pc),
	recvIndex(recvIndex),
	recvThread(INVALID_HANDLE_VALUE)
{}

TCPServ::ClientData::ClientData(ClientData&& clint)
	:
	pc(std::move(clint.pc)),
	func(clint.func),
	pingHandler(std::move(clint.pingHandler)),
	user(std::move(clint.user)),
	recvIndex(clint.recvIndex),
	recvThread(clint.recvThread)
{
	ZeroMemory(&clint, sizeof(ClientData));
}

TCPServ::ClientData& TCPServ::ClientData::operator=(ClientData&& data)
{
	if(this != &data)
	{
		pc = std::move(data.pc);
		func = data.func;
		pingHandler = std::move(data.pingHandler);
		user = std::move(data.user);
		recvIndex = data.recvIndex;
		recvThread = data.recvThread;

		ZeroMemory(&data, sizeof(ClientData));
	}
	return *this;
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
				char msg[] = { TYPE_CHANGE, MSG_CHANGE_SERVERFULL };
				HANDLE hnd = serv.SendClientData(msg, MSG_OFFSET, temp, true);
				TCPServ::WaitAndCloseHandle(hnd);

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
	TCPServ::ClientData**& clients = serv.GetClients();
	USHORT& pos = clients[data->pos]->recvIndex;
	TCPServ::ClientData* clint = clients[pos];
	Socket& pc = clint->pc;
	DWORD nBytesComp = 0, nBytesDecomp = 0;

	while(pc.IsConnected())
	{
		if(pc.ReadData(&nBytesDecomp, sizeof(DWORD)) > 0)
		{
			if(pc.ReadData(&nBytesComp, sizeof(DWORD)) > 0)
			{
				BYTE* compBuffer = alloc<BYTE>(nBytesComp);
				if(pc.ReadData(compBuffer, nBytesComp) > 0)
				{
					BYTE* deCompBuffer = alloc<BYTE>(nBytesDecomp);
					FileMisc::Decompress(deCompBuffer, nBytesDecomp, compBuffer, nBytesComp);
					dealloc(compBuffer);

					(clint->func)(&serv, clint, deCompBuffer, nBytesDecomp, serv.GetObj());
					dealloc(deCompBuffer);

					clint->pingHandler.SetPingTimer(serv, pc, serv.GetPingInterval());
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

	serv.RemoveClient(pos);
	destruct(data);
	return 0;
}


static DWORD CALLBACK SendData( LPVOID info )
{
	SData* data = (SData*)info;
	SAData* saData = data->saData;
	Socket pc = data->pc;

	if(pc.SendData(&saData->nBytesDecomp, sizeof(DWORD)) > 0)
		if(pc.SendData(&saData->nBytesComp, sizeof(DWORD)) > 0)
			pc.SendData(saData->data, saData->nBytesComp);

	//No need to remove client, that is handled in recv thread
	destruct(data);

	return 0;
}

static DWORD CALLBACK SendDataEx(LPVOID info)
{
	SDataEx* data = (SDataEx*)info;
	SADataEx* saData = data->saData;
	Socket pc = data->pc;

	if(pc.SendData(&saData->nBytesDecomp, sizeof(DWORD)) > 0)
		if(pc.SendData(&saData->nBytesComp, sizeof(DWORD)) > 0)
			pc.SendData(saData->data, saData->nBytesComp);

	//No need to remove client, that is handled in recv thread
	destruct(data);

	return 0;
}


static DWORD CALLBACK SendAllData( LPVOID info )
{
	SAData* data = (SAData*)info;
	TCPServ& serv = data->serv;
	auto& clients = serv.GetClients();
	void* dataDeComp = data->data;

	const DWORD nBytesComp = FileMisc::GetCompressedBufferSize(data->nBytesDecomp);
	BYTE* dataComp = alloc<BYTE>(nBytesComp);
	data->nBytesComp = FileMisc::Compress(dataComp, nBytesComp, (const BYTE*)dataDeComp, data->nBytesDecomp, data->serv.GetCompression());
	data->data = (char*)dataComp;

	HANDLE* hnds;
	USHORT nHnds = 0;

	if(data->single)
	{
		hnds = alloc<HANDLE>();
		if(data->exAddr.IsConnected())
			hnds[nHnds++] = CreateThread(NULL, 0, SendData, construct<SData>(SData(data, data->exAddr)), CREATE_SUSPENDED, NULL);
	}

	else
	{
		const USHORT nClients = serv.ClientCount();
		if(data->exAddr.IsConnected())
		{
			hnds = alloc<HANDLE>(nClients - 1);
			for(USHORT i = 0; i < nClients; i++)
				if(clients[i]->pc != data->exAddr)
					hnds[nHnds++] = CreateThread(NULL, 0, SendData, construct<SData>(SData(data, clients[i]->pc)), CREATE_SUSPENDED, NULL);
		}
		else
		{
			hnds = alloc<HANDLE>(nClients);
			for(USHORT i = 0; i < nClients; i++)
				hnds[nHnds++] = CreateThread(NULL, 0, SendData, construct<SData>(SData(data, clients[i]->pc)), CREATE_SUSPENDED, NULL);
		}
	}

	EnterCriticalSection(serv.GetSendSect());

	for(USHORT i = 0; i < nHnds; i++)
		ResumeThread(hnds[i]);

	WaitForMultipleObjects(nHnds, hnds, true, INFINITE);

	LeaveCriticalSection(serv.GetSendSect());

	for(USHORT i = 0; i < nHnds; i++)
		CloseHandle(hnds[i]);

	dealloc(hnds);
	dealloc(dataComp);
	destruct(data);

	return 0;
}

static DWORD CALLBACK SendAllDataEx( LPVOID info )
{
	SADataEx* data = (SADataEx*)info;
	TCPServ& serv = data->serv;
	Socket* pcs = data->pcs;
	void* dataDeComp = data->data;

	const DWORD nBytesComp = FileMisc::GetCompressedBufferSize(data->nBytesDecomp);
	BYTE* dataComp = alloc<BYTE>(nBytesComp);
	data->nBytesComp = FileMisc::Compress(dataComp, nBytesComp, (const BYTE*)dataDeComp, data->nBytesDecomp, data->serv.GetCompression());
	data->data = (char*)dataComp;

	const USHORT count = data->nPcs;
	HANDLE* hnds = alloc<HANDLE>(count);
	USHORT nHnds = 0;

	for(USHORT i = 0; i < count; i++)
	{
		if (pcs[i].IsConnected())
			hnds[nHnds++] = CreateThread(NULL, 0, SendDataEx, construct<SDataEx>(SDataEx(data, pcs[i])), CREATE_SUSPENDED, NULL);
	}

	EnterCriticalSection(serv.GetSendSect());

	for(USHORT i = 0; i < nHnds; i++)
		ResumeThread(hnds[i]);

	WaitForMultipleObjects(nHnds, hnds, true, INFINITE);

	LeaveCriticalSection(serv.GetSendSect());

	for(USHORT i = 0; i < nHnds; i++)
		CloseHandle(hnds[i]);

	dealloc(hnds);
	dealloc(dataComp);
	destruct(data);

	return 0;
}


HANDLE TCPServ::SendClientData(char* data, DWORD nBytes, Socket exAddr, bool single)
{
	return CreateThread(NULL, 0, &SendAllData, (LPVOID)construct<SAData>(SAData(*this, data, nBytes, exAddr, single)), NULL, NULL);
}

HANDLE TCPServ::SendClientData(char* data, DWORD nBytes, Socket* pcs, USHORT nPcs)
{
	return CreateThread(NULL, 0, &SendAllDataEx, (LPVOID)construct<SADataEx>(SADataEx(*this, data, nBytes, pcs, nPcs)), NULL, NULL);
}

HANDLE TCPServ::SendClientData(char* data, DWORD nBytes, std::vector<Socket>& pcs)
{
	return SendClientData(data, nBytes, pcs.data(), pcs.size());
}


void TCPServ::SendMsg(Socket pc, bool single, char type, char message)
{
	char msg[] = { type, message };

	HANDLE hnd = SendClientData(msg, MSG_OFFSET, pc, single);
	TCPServ::WaitAndCloseHandle(hnd);
}

void TCPServ::SendMsg(Socket* pcs, USHORT nPcs, char type, char message)
{
	char msg[] = { type, message };

	HANDLE hnd = SendClientData(msg, MSG_OFFSET, pcs, nPcs);
	TCPServ::WaitAndCloseHandle(hnd);
}

void TCPServ::SendMsg(std::vector<Socket>& pcs, char type, char message)
{
	SendMsg(pcs.data(), pcs.size(), type, message);
}

void TCPServ::SendMsg(std::tstring& user, char type, char message)
{
	for(USHORT i = 0; i < nClients; i++)
	{
		if(clients[i]->user.compare(user) == 0)
		{
			char msg[] = { type, message };

			HANDLE hnd = SendClientData(msg, MSG_OFFSET, clients[i]->pc, true);
			TCPServ::WaitAndCloseHandle(hnd);
		}
	}
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
	pingInterval(pingInterval)
{}

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
	pingInterval(serv.pingInterval)
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
		const_cast<void( *& )(ClientData&)>(conFunc) = serv.conFunc;
		const_cast<void( *& )(ClientData&)>(disFunc) = serv.disFunc;
		clientSect = serv.clientSect;
		sendSect = serv.sendSect;
		openCon = serv.openCon;
		const_cast<int&>(compression) = serv.compression;
		const_cast<USHORT&>(maxCon) = serv.maxCon;
		pingInterval = serv.pingInterval;

		ZeroMemory(&serv, sizeof(TCPServ));
	}
	return *this;
}

TCPServ::~TCPServ()
{
	Shutdown();
}

bool TCPServ::AllowConnections(const TCHAR* port)
{
	host.Bind(port);

	if(!host.IsConnected())
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

	Data* data = construct<Data>(Data(*this, nClients));
	clients[nClients] = construct<ClientData>({ data->serv, pc, function, data->pos });
	clients[nClients]->recvThread = CreateThread(NULL, 0, RecvData, data, NULL, NULL);

	RunConFunc(*clients[nClients]);

	++nClients;

	LeaveCriticalSection(&clientSect);
}

void TCPServ::RemoveClient(USHORT& pos)
{
	EnterCriticalSection(&clientSect);

	const USHORT index = pos;

	ClientData& data = *clients[index];
	RunDisFunc(data);

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

	if(!user.empty())//if user wasnt declined authentication
	{
		MsgStreamWriter streamWriter(TYPE_CHANGE, MSG_CHANGE_DISCONNECT, (user.size() + 1) * sizeof(TCHAR));
		streamWriter.WriteEnd(user.c_str());
		HANDLE hnd = SendClientData(streamWriter, streamWriter.GetSize(), Socket(), false);
		WaitAndCloseHandle(hnd);
	}
}

TCPServ::ClientData* TCPServ::FindClient(std::tstring &user)
{
	for (USHORT i = 0; i < nClients; i++)
	{
		if (clients[i]->user == user)
		{
			return clients[i];
		}
	}
	return nullptr;
}

void TCPServ::Shutdown()
{
	if(clients)
	{
		host.Disconnect();//causes opencon thread to close
		WaitAndCloseHandle(openCon);

		//close recv threads and free memory
		for(USHORT i = 0; i < nClients; i++)
		{
			ClientData& data = *clients[i];
			data.pc.Disconnect();
			WaitForSingleObject(data.recvThread, INFINITE); //handle closed in RemoveClient
			destruct(clients[i]);
		}

		dealloc(clients);

		DeleteCriticalSection(&clientSect);
		DeleteCriticalSection(&sendSect);
	}
}

void TCPServ::WaitAndCloseHandle(HANDLE& hnd)
{
	DWORD temp = WaitForSingleObject(hnd, INFINITE);
	CloseHandle(hnd);
	hnd = NULL; //NULL instead of INVALID_HANDLE_VALUE due to move ctor
}

void TCPServ::Ping(Socket& client)
{
	char msg[] = { TYPE_PING, MSG_PING };
	HANDLE hnd = SendClientData(msg, MSG_OFFSET, client, true);
	WaitAndCloseHandle(hnd);
}

Socket& TCPServ::GetHost()
{
	return host;
}

bool TCPServ::MaxClients() const
{
	return nClients == maxCon;
}

TCPServ::ClientData**& TCPServ::GetClients()
{
	return clients;
}

USHORT TCPServ::ClientCount() const
{
	return nClients;
}

void TCPServ::SetFunction(USHORT index, sfunc function)
{
	clients[index]->func = function;
}

void TCPServ::SetPingInterval(float interval)
{
	this->pingInterval = interval;
}

void TCPServ::RunConFunc(ClientData& client)
{
	conFunc(client);
}

void TCPServ::RunDisFunc(ClientData& client)
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

float TCPServ::GetPingInterval() const
{
	return pingInterval;
}
