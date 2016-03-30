#include "StdAfx.h"
#include "TCPServ.h"
#include "CNLIB/BufSize.h"
#include "CNLIB/File.h"
#include "CNLIB/Messages.h"
#include "CNLIB/MsgHeader.h"

TCPServInterface* CreateServer(sfunc msgHandler, ConFunc conFunc, DisconFunc disFunc, UINT maxPCSendOps, UINT maxDataBuffSize, UINT singleOlPCCount, UINT allOlCount, UINT sendBuffCount, UINT sendCompBuffCount, UINT sendMsgBuffCount, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, SocketOptions sockOpts, void* obj)
{
	return construct<TCPServ>(msgHandler, conFunc, disFunc, maxPCSendOps, maxDataBuffSize, singleOlPCCount, allOlCount, sendBuffCount, sendCompBuffCount, sendMsgBuffCount, maxCon, compression, compressionCO, keepAliveInterval, sockOpts, obj);
}
void DestroyServer(TCPServInterface*& server)
{
	destruct((TCPServ*&)server);
}


ClientAccess::ClientAccess(ClientData** clients)
	:
	clients(clients)
{}
ClientData* ClientAccess::operator+(UINT amount)
{
	return *(ClientData**)((ClientDataEx**)clients + amount);
}
ClientData* ClientAccess::operator[](UINT index)
{
	return *(ClientData**)((ClientDataEx**)clients + index);
}

ClientData::ClientData(Socket pc, sfunc func)
	:
	pc(pc),
	func(func),
	obj(nullptr)
{}
ClientData::ClientData(ClientData&& clint)
	:
	pc(std::move(clint.pc)),
	func(clint.func),
	user(std::move(clint.user)),
	obj(clint.obj)
{
	ZeroMemory(&clint, sizeof(ClientData));
}
ClientData& ClientData::operator=(ClientData&& data)
{
	if(this != &data)
	{
		pc = std::move(data.pc);
		func = data.func;
		user = std::move(data.user);
		obj = data.obj;

		ZeroMemory(&data, sizeof(ClientData));
	}
	return *this;
}
UINT ClientData::GetOpCount() const
{
	return ((ClientDataEx*)this)->opCount.load();
}


ClientDataEx::ClientDataEx(TCPServ& serv, Socket pc, sfunc func, UINT arrayIndex)
	:
	ClientData(pc, func),
	serv(serv),
	ol(OpType::recv),
	arrayIndex(arrayIndex),
	opCount(1), // for recv
	olPool(sizeof(OverlappedSendSingle), serv.SingleOlPCCount()),
	recvHandler(serv.GetBufferOptions(), 3, &serv),
	opsPending(),
	state(running)
{
	const UINT maxDataSize = serv.GetBufferOptions().GetMaxDataSize() + sizeof(MsgHeader);
}
ClientDataEx::ClientDataEx(ClientDataEx&& clint)
	:
	ClientData(std::forward<ClientData>(*this)),
	serv(clint.serv),
	ol(clint.ol),
	arrayIndex(clint.arrayIndex),
	opCount(clint.opCount.load()),
	olPool(std::move(clint.olPool)),
	recvHandler(std::move(clint.recvHandler)),
	opsPending(std::move(clint.opsPending)),
	state(clint.state)
{
	ZeroMemory(&clint, sizeof(ClientDataEx));
}
ClientDataEx& TCPServ::ClientDataEx::operator=(ClientDataEx&& data)
{
	if (this != &data)
	{
		__super::operator=(std::forward<ClientData>(data));
		arrayIndex = data.arrayIndex;
		ol = data.ol;
		opCount = data.opCount.load();
		olPool = std::move(data.olPool);
		recvHandler = std::move(data.recvHandler);
		opsPending = std::move(data.opsPending);
		state = data.state;

		memset(&data, 0, sizeof(ClientDataEx));
	}

	return *this;
}
ClientDataEx::~ClientDataEx(){}

bool ClientDataEx::DecRefCount()
{
	return --opCount == 0;
}
void ClientDataEx::FreePendingOps()
{
	//Cleanup all pending operations
	if (!opsPending.empty())
	{
		lock.Lock();

		for (int i = 0, size = opsPending.size(); i < size; i++)
		{
			serv.SendDataSingleCR(*this, opsPending.front());
			opsPending.pop();
		}

		lock.Unlock();
	}
}
void ClientDataEx::Cleanup()
{
	pc.Disconnect();
	user.clear();
}
void ClientDataEx::Reset(const Socket& pc, UINT arrayIndex)
{
	this->pc = pc;
	this->arrayIndex = arrayIndex;
	if (opCount._My_val != 1)
		opCount = 1;
	state = running;
}


HostSocket::HostSocket(TCPServ& serv)
	:
	serv(serv),
	listen(),
	acceptData(nullptr),
	nConcAccepts(0)
{}

HostSocket::HostSocket(HostSocket&& host)
	:
	serv(host.serv),
	listen(std::move(host.listen)),
	acceptData(host.acceptData),
	nConcAccepts(host.nConcAccepts)
{
	memset(&host, 0, sizeof(HostSocket));
}
HostSocket& HostSocket::operator=(HostSocket&& host)
{
	if (this != &host)
	{
		serv = std::move(host.serv);
		listen = std::move(host.listen);
		acceptData = host.acceptData;
		nConcAccepts = host.nConcAccepts;

		memset(&host, 0, sizeof(HostSocket));
	}
	return *this;
}

void HostSocket::Initalize(UINT nConcAccepts)
{
	this->nConcAccepts = nConcAccepts;
	acceptData = alloc<AcceptData>(nConcAccepts);

	for (UINT i = 0; i < nConcAccepts; i++)
		acceptData[i].Initalize(this);
}
void HostSocket::Cleanup()
{
	for (UINT i = 0; i < nConcAccepts; i++)
		acceptData[i].Cleanup();

	dealloc(acceptData);
}

bool HostSocket::Bind(const LIB_TCHAR* port, bool ipv6)
{
	return listen.Bind(port, ipv6);
}
bool HostSocket::QueueAccept()
{
	bool b = true;
	for (int i = 0; i < nConcAccepts; i++)
		b &= acceptData[i].QueueAccept();

	return b;
}
bool HostSocket::LinkIOCP(IOCP& iocp)
{
	//for (UINT i = 0; i < nConcAccepts; i++)
	return iocp.LinkHandle((HANDLE)listen.GetSocket(), this);
}
void HostSocket::Disconnect()
{
	listen.Disconnect();
}

TCPServ& HostSocket::GetServ()
{
	return serv;
}
const Socket& HostSocket::GetListen() const
{
	return listen;
}


HostSocket::AcceptData::AcceptData()
	:
	hostSocket(nullptr),
	accept(NULL),
	buffer(nullptr),
	ol(this)
{}
HostSocket::AcceptData::AcceptData(AcceptData&& host)
	:
	hostSocket(host.hostSocket),
	accept(host.accept),
	buffer(host.buffer),
	ol(host.ol)
{
	memset(&host, 0, sizeof(AcceptData));
}

HostSocket::AcceptData& HostSocket::AcceptData::operator=(AcceptData&& host)
{
	if (this != &host)
	{
		hostSocket = host.hostSocket;
		accept = host.accept;
		buffer = host.buffer;
		ol = host.ol;
	}

	return *this;
}

void HostSocket::AcceptData::Initalize(HostSocket* hostSocket)
{
	ol.opType = OpType::accept;
	ol.acceptData = this;
	ol.Reset();

	this->hostSocket = hostSocket;
	buffer = alloc<char>(localAddrLen + remoteAddrLen);
	ResetAccept();
}
void HostSocket::AcceptData::Cleanup()
{
	closesocket(accept);
	dealloc(buffer);
}

void HostSocket::AcceptData::ResetAccept()
{
	accept = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
}
bool HostSocket::AcceptData::QueueAccept()
{
	return hostSocket->listen.AcceptOl(accept, buffer, localAddrLen, remoteAddrLen, &ol);
}
void HostSocket::AcceptData::AcceptConCR()
{
	hostSocket->GetServ().AcceptConCR(*this);
}

SOCKET HostSocket::AcceptData::GetAccept() const
{
	return accept;
}
TCPServ::OverlappedAccept* HostSocket::AcceptData::GetOl()
{
	return &ol;
}
HostSocket* HostSocket::AcceptData::GetHostSocket() const
{
	return hostSocket;
}
void HostSocket::AcceptData::GetAcceptExAddrs(sockaddr** local, int* localLen, sockaddr** remote, int* remoteLen)
{
	Socket::GetAcceptExAddrs(buffer, localAddrLen, remoteAddrLen, local, localLen, remote, remoteLen);
}

static DWORD CALLBACK IOCPThread(LPVOID info)
{
	IOCP& iocp = *(IOCP*)info;
	DWORD bytesTrans = 0;
	OverlappedExt* ol = nullptr;
	ULONG_PTR key = NULL;
	
	bool res = false;
	do
	{
		res = GetQueuedCompletionStatus(iocp.GetHandle(), &bytesTrans, &key, (OVERLAPPED**)&ol, INFINITE);
		if (res)
		{
			if (!ol)
				break;
			switch (ol->opType)
			{
				case OpType::recv:
				{
					ClientDataEx& cd = *(ClientDataEx*)key;
					if (bytesTrans == 0)
					{
						cd.serv.RemoveClient(&cd, cd.state != ClientDataEx::closing);
						continue;
					}

					if (!cd.recvHandler.RecvDataCR(cd.pc, bytesTrans, cd.serv.GetBufferOptions(), (void*)key) && cd.DecRefCount())
						cd.serv.RemoveClient(&cd, cd.state != ClientDataEx::closing);
				}
				break;
				case OpType::send:
				{
					ClientDataEx& cd = *(ClientDataEx*)key;
					cd.serv.SendDataCR(cd, (OverlappedSend*)ol);
				}
				break;
				case OpType::sendsingle:
				{
					ClientDataEx& cd = *(ClientDataEx*)key;
					cd.serv.SendDataSingleCR(cd, (OverlappedSendSingle*)ol);
				}
				break;
				case OpType::accept:
				{
					((TCPServ::OverlappedAccept*)ol)->acceptData->AcceptConCR();
				}
				break;
			}
		}
		else
		{
			if (ol && key)
			{
				//temporary fix
				res = true;
				if (ol->opType == OpType::send)
				{
					ClientDataEx& cd = *(ClientDataEx*)key;
					cd.serv.SendDataCR(cd, (OverlappedSend*)ol);
				}
				else if (ol->opType == OpType::sendsingle)
				{
					ClientDataEx& cd = *(ClientDataEx*)key;
					cd.serv.SendDataSingleCR(cd, (OverlappedSendSingle*)ol);
				}
				else if (ol->opType == OpType::recv)
				{
					ClientDataEx* cd = (ClientDataEx*)key;
					if (cd->DecRefCount())
						cd->serv.RemoveClient(cd, cd->state != ClientDataEx::closing); //closing flag only set when you initiated close
				}
				else
				{
					HostSocket& hostSocket = *(HostSocket*)key;
					HostSocket::AcceptData::OverlappedAccept* olAccept = (HostSocket::AcceptData::OverlappedAccept*)ol;
					hostSocket.GetServ().CleanupAcceptEx(*(olAccept->acceptData));
					/*HostSocket& hostSocket = *(HostSocket*)key;
					hostSocket.serv.CleanupAcceptEx(hostSocket);*/
				}
			}
			//else
			//{
			//	//Error
			//}
		}
	} while (res);

	return 0;
}


void TCPServ::OnNotify(char* data, DWORD nBytes, void* cd)
{
	(((ClientData*)cd)->func)(*this, (ClientData*)cd, MsgStreamReader{ data, nBytes - MSG_OFFSET });
}

void TCPServ::FreeSendOlInfo(OverlappedSendInfo* ol)
{
	bufSendAlloc.FreeBuff(ol->sendBuff);
	sendOlPoolAll.dealloc(ol->head);
	sendOlInfoPool.dealloc(ol);
}
void TCPServ::FreeSendOlSingle(ClientDataEx& client, OverlappedSendSingle* ol)
{
	bufSendAlloc.FreeBuff(ol->sendBuff);
	client.olPool.dealloc(ol);
}

BuffSendInfo TCPServ::GetSendBuffer(DWORD hiByteEstimate, CompressionType compType)
{
	return bufSendAlloc.GetSendBuffer(hiByteEstimate);
}
BuffSendInfo TCPServ::GetSendBuffer(BuffAllocator* alloc, DWORD nBytes, CompressionType compType)
{
	return bufSendAlloc.GetSendBuffer(alloc, nBytes, compType);
}

MsgStreamWriter TCPServ::CreateOutStream(DWORD hiByteEstimate, short type, short msg, CompressionType compType)
{
	return bufSendAlloc.CreateOutStream(hiByteEstimate, type, msg, compType);
}
MsgStreamWriter TCPServ::CreateOutStream(BuffAllocator* alloc, DWORD nBytes, short type, short msg, CompressionType compType)
{
	return bufSendAlloc.CreateOutStream(alloc, nBytes, type, msg, compType);
}

const BufferOptions TCPServ::GetBufferOptions() const
{
	return bufSendAlloc.GetBufferOptions();
}

bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientData* exClient, bool single)
{
	return SendClientData(buffSendInfo, nBytes, (ClientDataEx*)exClient, single, false);
}
bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientData** clients, UINT nClients)
{
	return SendClientData(buffSendInfo, nBytes, (ClientDataEx**)clients, nClients, false);
}
bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, std::vector<ClientData*>& pcs)
{
	return SendClientData(buffSendInfo, nBytes, pcs.data(), pcs.size());
}

bool TCPServ::SendClientData(const MsgStreamWriter& streamWriter, ClientData* exClient, bool single)
{
	return SendClientData(streamWriter.GetBuffSendInfo(), streamWriter.GetSize(), (ClientDataEx*)exClient, single, false);
}
bool TCPServ::SendClientData(const MsgStreamWriter& streamWriter, ClientData** clients, UINT nClients)
{
	return SendClientData(streamWriter.GetBuffSendInfo(), streamWriter.GetSize(), (ClientDataEx**)clients, nClients, false);
}
bool TCPServ::SendClientData(const MsgStreamWriter& streamWriter, std::vector<ClientData*>& pcs)
{
	return SendClientData(streamWriter.GetBuffSendInfo(), streamWriter.GetSize(), pcs.data(), pcs.size());
}

bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientDataEx* exClient, bool single, bool msg)
{
	return SendClientData(bufSendAlloc.CreateBuff(buffSendInfo, nBytes, msg), exClient, single);
}
bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientDataEx** clients, UINT nClients, bool msg)
{
	return SendClientData(bufSendAlloc.CreateBuff(buffSendInfo, nBytes, msg), clients, nClients);
}

bool TCPServ::SendClientSingle(ClientDataEx& clint, OverlappedSendSingle* ol, bool popQueue)
{
	if (clint.pc.IsConnected())
	{
		if (clint.opCount.load() < maxPCSendOps)
		{
			++clint.opCount;
			++opCounter;

			long res = clint.pc.SendDataOl(&ol->sendBuff, ol);
			long err = WSAGetLastError();
			if ((res == SOCKET_ERROR) && (err != WSA_IO_PENDING))
			{
				--opCounter;
				--clint.opCount;

				//if (err == WSAENOBUFS)
				//{
				//	//queue the send for later
				//	clint.opsPending.emplace(ol);
				//}
				//else
				//{
					FreeSendOlSingle(clint, ol);
				//}
			}
			else if (popQueue)
			{
				//okay to lock and unlock here
				clint.lock.Lock();

				//queue the send for later
				clint.opsPending.pop();

				clint.lock.Unlock();
			}
		}
		else if (!popQueue)
		{
			//queue the send for later
			clint.opsPending.emplace(ol);
		}
	}
	else
	{
		FreeSendOlSingle(clint, ol);

		return false;
	}

	return true;
}
bool TCPServ::SendClientData(const WSABufSend& sendBuff, ClientDataEx* exClient, bool single)
{
	long res = 0, err = 0;

	if (!sendBuff.head)
		return false;

	if (single)
	{
		if (!(exClient && exClient->pc.IsConnected()))
		{
			bufSendAlloc.FreeBuff((WSABufSend&)sendBuff);
			return false;
		}

		return SendClientSingle(*exClient, exClient->olPool.construct<OverlappedSendSingle>(sendBuff));
	}
	else
	{
		if (!nClients || (nClients == 1 && exClient))
		{
			bufSendAlloc.FreeBuff((WSABufSend&)sendBuff);
			return false;
		}

		OverlappedSendInfo* sendInfo = sendOlInfoPool.construct<OverlappedSendInfo>(sendBuff, nClients);
		OverlappedSend* ol = sendOlPoolAll.construct<OverlappedSend>(sendInfo);
		sendInfo->head = ol;

		//UINT globalOpIndex = ++opIndex;

		opCounter += nClients;
		for (UINT i = 0; i < nClients; i++)
		{
			ClientDataEx* clint = clients[i];

			ol[i].Initalize(sendInfo);
			if (clint != exClient)
			{
				++clint->opCount;

				res = clint->pc.SendDataOl(&sendInfo->sendBuff, &ol[i]);
				err = WSAGetLastError();
			}

			if (clint == exClient || ((res == SOCKET_ERROR) && (err != WSA_IO_PENDING)))
			{
				//if (err == WSAENOBUFS)
				//{
				//	//queue the send for later
				//	++clint->opCount;
				//	clint->opsPending.emplace(&ol[i]);
				//}
				//else
				//{
				if (clint != exClient)
					--clint->opCount;

					--opCounter;
					if (sendInfo->DecrementRefCount())
						FreeSendOlInfo(sendInfo);
				//}
			}
		}
	}
	return true;
}
bool TCPServ::SendClientData(const WSABufSend& sendBuff, ClientDataEx** clients, UINT nClients)
{
	long res = 0, err = 0;

	if (!sendBuff.head)
		return false;

	if (!(clients && nClients))
	{
		bufSendAlloc.FreeBuff((WSABufSend&)sendBuff);
		return false;
	}

	if (nClients == 1)
	{
		ClientDataEx& clint = **clients;

		return SendClientSingle(clint, clint.olPool.construct<OverlappedSendSingle>(sendBuff));
	}
	else
	{
		OverlappedSendInfo* sendInfo = sendOlInfoPool.construct<OverlappedSendInfo>(sendBuff, nClients);
		OverlappedSend* ol = sendOlPoolAll.construct<OverlappedSend>(sendInfo);
		sendInfo->head = ol;

		opCounter += nClients;
		for (UINT i = 0; i < nClients; i++)
		{
			ClientDataEx& clint = **(clients + i);
			ol[i].Initalize(sendInfo);
			++clint.opCount;

			res = clint.pc.SendDataOl(&sendInfo->sendBuff, &ol[i]);
			err = WSAGetLastError();

			if ((res == SOCKET_ERROR) && (err != WSA_IO_PENDING))
			{
				//if (err == WSAENOBUFS)
				//{
				//	//queue the send for later
				//	++clint.opCount;
				//	clint.opsPending.emplace(&ol[i]);
				//}
				//else
				//{
					--clint.opCount;
					--opCounter;
					if (sendInfo->DecrementRefCount())
						FreeSendOlInfo(sendInfo);
				//}
			}
		}
	}
	return true;
}

void TCPServ::SendMsg(ClientData* exClient, bool single, short type, short message)
{
	short msg[] = { type, message };

	SendClientData({ (char*)msg, NOCOMPRESSION, 0 }, MSG_OFFSET, (ClientDataEx*)exClient, single, true);
}
void TCPServ::SendMsg(ClientData** clients, UINT nClients, short type, short message)
{
	short msg[] = { type, message };

	SendClientData({ (char*)msg, NOCOMPRESSION, 0 }, MSG_OFFSET, (ClientDataEx**)clients, nClients, true);
}
void TCPServ::SendMsg(std::vector<ClientData*>& pcs, short type, short message)
{
	SendMsg(pcs.data(), (USHORT)pcs.size(), type, message);
}
void TCPServ::SendMsg(const std::tstring& user, short type, short message)
{
	SendMsg((ClientDataEx*)FindClient(user), true, type, message);
}


void TCPServ::AcceptConCR(HostSocket::AcceptData& acceptData)
{
	sockaddr *localAddr, *remoteAddr;
	int localLen, remoteLen;

	//Separate addresses and create new socket
	acceptData.GetAcceptExAddrs(&localAddr, &localLen, &remoteAddr, &remoteLen);
	//Socket::GetAcceptExAddrs(host.buffer, host.localAddrLen, host.remoteAddrLen, &localAddr, &localLen, &remoteAddr, &remoteLen);
	Socket socket = acceptData.GetAccept();
	socket.SetAddrInfo(remoteAddr, false);

	//Get ready for another call
	acceptData.ResetAccept();

	//Queue another acceptol
	//host.listen.AcceptOl(host.accept, host.buffer, host.localAddrLen, host.remoteAddrLen, ol);
	acceptData.QueueAccept();

	//If server is not full and has passed the connection condition test
	if (!MaxClients() && connectionCondition(*this, socket))
		AddClient(socket); //Add client to server list
	else
		socket.Disconnect();
}
void TCPServ::SendDataCR(ClientDataEx& cd, OverlappedSend* ol)
{
	OverlappedSendInfo* sendInfo = ol->sendInfo;
	if (sendInfo->DecrementRefCount())
		FreeSendOlInfo(sendInfo);

	if (cd.DecRefCount())
		RemoveClient(&cd, cd.state != ClientDataEx::closing);

	if (--opCounter == 0)
		SetEvent(shutdownEv);
}	
void TCPServ::SendDataSingleCR(ClientDataEx& cd, OverlappedSendSingle* ol)
{
	FreeSendOlSingle(cd, ol);

	//If there are per client operations pending then attempt to complete them
	if (!cd.opsPending.empty() && cd.state != cd.closing)
	{
		cd.lock.Lock();
		SendClientSingle(cd, cd.opsPending.front(), true);
		cd.lock.Unlock();
	}

	if (cd.DecRefCount())
		RemoveClient(&cd, cd.state != ClientDataEx::closing);

	if (--opCounter == 0)
		SetEvent(shutdownEv);
}
void TCPServ::CleanupAcceptEx(HostSocket::AcceptData& acceptData)
{
	acceptData.Cleanup();

	if (--opCounter == 0)
		SetEvent(shutdownEv);
}


TCPServ::TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, UINT maxPCSendOps, UINT maxDataBuffSize, UINT singleOlPCCount, UINT allOlCount, UINT sendBuffCount, UINT sendCompBuffCount, UINT sendMsgBuffCount, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, SocketOptions sockOpts, void* obj)
	:
	ipv4Host(*this),
	ipv6Host(*this),
	clients(nullptr),
	nClients(0),
	iocp(nullptr),
	function(func),
	conFunc(conFunc),
	connectionCondition(nullptr),
	disFunc(disFunc),
	singleOlPCCount(singleOlPCCount),
	maxPCSendOps(maxPCSendOps + 1),//+1 for recv
	maxCon(maxCon),
	keepAliveInterval(keepAliveInterval),
	keepAliveHandler(nullptr),
	bufSendAlloc(maxDataBuffSize, sendBuffCount, sendCompBuffCount, sendMsgBuffCount, compression, compressionCO),
	clientPool(sizeof(ClientDataEx), maxCon),
	sendOlInfoPool(sizeof(OverlappedSendInfo), allOlCount),
	sendOlPoolAll(sizeof(OverlappedSend) * maxCon, allOlCount),
	opCounter(0),
	shutdownEv(NULL),
	sockOpts(sockOpts),
	obj(obj),
	shuttingDown(false)
{
	clients = alloc<ClientDataEx*>(maxCon);

	for (UINT i = 0; i < maxCon; i++)
		clients[i] = clientPool.construct<ClientDataEx>(*this, INVALID_SOCKET, function, 0);

}
TCPServ::TCPServ(TCPServ&& serv)
	:
	ipv4Host(std::move(serv.ipv4Host)),
	ipv6Host(std::move(serv.ipv6Host)),
	clients(serv.clients), 
	nClients(serv.nClients),
	iocp(std::move(serv.iocp)),
	function(serv.function),
	conFunc(serv.conFunc),
	connectionCondition(serv.connectionCondition),
	disFunc(serv.disFunc),
	clientLock(std::move(serv.clientLock)),
	singleOlPCCount(serv.singleOlPCCount),
	maxPCSendOps(serv.maxPCSendOps),
	maxCon(serv.maxCon),
	keepAliveHandler(std::move(serv.keepAliveHandler)),
	bufSendAlloc(std::move(serv.bufSendAlloc)),
	clientPool(std::move(serv.clientPool)),
	sendOlInfoPool(std::move(serv.sendOlInfoPool)),
	sendOlPoolAll(std::move(serv.sendOlPoolAll)),
	opCounter(serv.opCounter.load()),
	shutdownEv(serv.shutdownEv),
	sockOpts(serv.sockOpts),
	obj(serv.obj),
	shuttingDown(serv.shuttingDown)
{
	ZeroMemory(&serv, sizeof(TCPServ));
}
TCPServ& TCPServ::operator=(TCPServ&& serv)
{
	if(this != &serv)
	{
		this->~TCPServ();

		ipv4Host = std::move(serv.ipv4Host);
		ipv6Host = std::move(serv.ipv6Host);
		clients = serv.clients;
		nClients = serv.nClients;
		iocp = std::move(serv.iocp);
		function = serv.function;
		const_cast<std::remove_const_t<ConFunc>&>(conFunc) = serv.conFunc;
		connectionCondition = serv.connectionCondition;
		const_cast<std::remove_const_t<DisconFunc>&>(disFunc) = serv.disFunc;
		clientLock = std::move(serv.clientLock);
		const_cast<UINT&>(singleOlPCCount) = serv.singleOlPCCount;
		const_cast<UINT&>(maxPCSendOps) = serv.maxPCSendOps;
		const_cast<UINT&>(maxCon) = serv.maxCon;
		keepAliveHandler = std::move(serv.keepAliveHandler);
		bufSendAlloc = std::move(serv.bufSendAlloc);
		clientPool = std::move(serv.clientPool);
		sendOlInfoPool = std::move(serv.sendOlInfoPool);
		sendOlPoolAll = std::move(serv.sendOlPoolAll);
		opCounter = serv.opCounter.load();
		shutdownEv = serv.shutdownEv;
		sockOpts = serv.sockOpts;
		obj = serv.obj;
		shuttingDown = serv.shuttingDown;

		ZeroMemory(&serv, sizeof(TCPServ));
	}
	return *this;
}

TCPServ::~TCPServ()
{
	Shutdown();

	//Free up resources used to create client array
	for (UINT i = 0; i < maxCon; i++)
		clientPool.destruct(clients[i]);

	//Free up client array
	dealloc(clients);
}

bool TCPServ::BindHost(HostSocket& host, const LIB_TCHAR* port, bool ipv6, UINT nConcAccepts)
{
	bool b = true;

	if (b = host.Bind(port, ipv6))
	{
		host.Initalize(nConcAccepts);
		if (b = host.LinkIOCP(*iocp))
		{
			host.QueueAccept();
			opCounter += nConcAccepts;
		}
	}

	return b;
}
IPv TCPServ::AllowConnections(const LIB_TCHAR* port, ConCondition connectionCondition, DWORD nThreads, DWORD nConcThreads, IPv ipv, UINT nConcAccepts)
{
	if(ipv4Host.GetListen().IsConnected() || ipv6Host.GetListen().IsConnected())
		return ipvnone;

	this->connectionCondition = connectionCondition;

	if (iocp)
		return ipvnone;

	iocp = construct<IOCP>(nThreads, nConcThreads, IOCPThread);

	if (ipv & ipv4 && !BindHost(ipv4Host, port, false, nConcAccepts))
		ipv = (IPv)(ipv ^ ipv4);

	if (ipv & ipv6 && !BindHost(ipv6Host, port, true, nConcAccepts))
		ipv = (IPv)(ipv ^ ipv6);
	
	if (keepAliveInterval != 0.0f)
	{
		keepAliveHandler = construct<KeepAliveHandler>(this);

		if (!keepAliveHandler)
			return ipvnone;
	}

	keepAliveHandler->SetKeepAliveTimer(keepAliveInterval);

	shutdownEv = CreateEvent(NULL, TRUE, FALSE, NULL);

	return ipv;
}

void TCPServ::AddClient(Socket pc)
{
	++opCounter; //For recv

	clientLock.Lock();

	ClientDataEx* cd = clients[nClients];
	cd->Reset(pc, nClients++);

	//ClientDataEx* cd = clients[nClients] = clientPool.construct<ClientDataEx>(*this, pc, function, nClients);
	//nClients += 1;

	clientLock.Unlock();

	if (sockOpts.NoDelay())
		pc.SetNoDelay(true);
	if (sockOpts.UseOwnSBuf())
		pc.SetTCPSendStack();
	if (sockOpts.UseOwnRBuf())
		pc.SetTCPRecvStack();

	iocp->LinkHandle((HANDLE)pc.GetSocket(), cd);

	cd->recvHandler.StartRead(pc);

	RunConFunc(cd);
}
void TCPServ::RemoveClient(ClientDataEx* client, bool unexpected)
{
	//Cleanup all pending operations
	client->FreePendingOps();

	clientLock.Lock();

	const UINT index = client->arrayIndex;
	ClientDataEx*& data = clients[index];

	if(index != (nClients - 1))
	{
		std::swap(clients[nClients - 1], data);
		data->arrayIndex = index;
	}

	nClients -= 1;

	clientLock.Unlock();

	RunDisFunc(client, unexpected);
	client->Cleanup();

	if (--opCounter == 0)
		SetEvent(shutdownEv);

	//if(!user.empty() && !(ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected()))//if user wasnt declined authentication, and server was not shut down
	//{
	//	MsgStreamWriter streamWriter(TYPE_CHANGE, MSG_CHANGE_DISCONNECT, StreamWriter::SizeType(user));
	//	streamWriter.Write(user);
	//	SendClientData(streamWriter, streamWriter.GetSize(), Socket(), false);
	//}
}

TCPServ::ClientData* TCPServ::FindClient(const std::tstring& user) const
{
	for (ClientDataEx **ptr = clients, **end = clients + nClients; ptr != end; ptr++)
	{
		ClientDataEx* cd = *ptr;
		if (cd && cd->user.compare(user) == 0)
			return *ptr;
	}
	return nullptr;
}

void TCPServ::DisconnectClient(ClientData* client)
{
	ClientDataEx* clint = (ClientDataEx*)client;
	clint->state = ClientDataEx::closing;
	clint->pc.Disconnect();
}

void TCPServ::Shutdown()
{
	if (shutdownEv)
	{
		shuttingDown = true;

		//Stop sending keepalive packets
		destruct(keepAliveHandler);

		//Stop accepting new connections
		ipv4Host.Disconnect();
		ipv6Host.Disconnect();

		//Cancel all outstanding operations
		for (ClientDataEx **ptr = clients, **end = clients + nClients; ptr != end; ptr++)
		{
			ClientDataEx* cd = *ptr;
			if (cd)
				cd->pc.Disconnect();
		}

		//Wait for all operations to cease
		WaitForSingleObject(shutdownEv, INFINITE);

		//Post a close message to all iocp threads
		iocp->Post(0, nullptr);

		//Wait for iocp threads to close, then cleanup iocp
		iocp->WaitAndCleanup();

		destruct(iocp);

		//Cleanup accept data
		ipv4Host.Cleanup();
		ipv6Host.Cleanup();

		CloseHandle(shutdownEv);
		shutdownEv = NULL;

		shuttingDown = false;
	}
}

void TCPServ::KeepAlive()
{
	for (ClientDataEx **ptr = clients, **end = clients + nClients; ptr != end; ptr++)
	{
		ClientDataEx* cd = *ptr;
		if (cd)
			cd->pc.SendData(nullptr, 0);
	}
}

ClientAccess TCPServ::GetClients() const
{
	return (ClientData**)clients;
}
UINT TCPServ::ClientCount() const
{
	return nClients;
}
UINT TCPServ::MaxClientCount() const
{
	return maxCon;
}

const Socket TCPServ::GetHostIPv4() const
{
	return ipv4Host.GetListen();
}
const Socket TCPServ::GetHostIPv6() const
{
	return ipv6Host.GetListen();
}

void TCPServ::SetKeepAliveInterval(float interval)
{
	keepAliveInterval = interval;
	keepAliveHandler->SetKeepAliveTimer(keepAliveInterval);
}
float TCPServ::GetKeepAliveInterval() const
{
	return keepAliveInterval;
}

void TCPServ::RunConFunc(ClientData* client)
{
	conFunc(*this, client);
}
void TCPServ::RunDisFunc(ClientData* client, bool unexpected)
{
	disFunc(*this, client, unexpected);
}

void* TCPServ::GetObj() const
{
	return obj;
}

bool TCPServ::MaxClients() const
{
	return nClients == maxCon;
}
bool TCPServ::IsConnected() const
{
	return ipv4Host.GetListen().IsConnected() || ipv6Host.GetListen().IsConnected();
}
bool TCPServ::ShuttingDown() const
{
	return shuttingDown;
}

const SocketOptions TCPServ::GetSockOpts() const
{
	return sockOpts;
}

UINT TCPServ::GetOpCount() const
{
	return opCounter.load();
}

UINT TCPServ::SingleOlPCCount() const
{
	return singleOlPCCount;
}
UINT TCPServ::GetMaxPcSendOps() const
{
	return maxPCSendOps;
}

IOCP& TCPServ::GetIOCP()
{
	return *iocp;
}