#include "StdAfx.h"
#include "TCPServ.h"
#include "CNLIB/BufSize.h"
#include "CNLIB/File.h"
#include "CNLIB/Messages.h"

TCPServInterface* CreateServer(sfunc msgHandler, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads, DWORD nConcThreads, UINT maxPCSendOps, UINT maxDataSize, UINT singleOlPCCount, UINT allOlCount, UINT sendBuffCount, UINT sendMsgBuffCount, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, SocketOptions sockOpts, void* obj)
{
	return construct<TCPServ>(msgHandler, conFunc, disFunc, nThreads, nConcThreads, maxPCSendOps, maxDataSize, singleOlPCCount, allOlCount, sendBuffCount, sendMsgBuffCount, maxCon, compression, compressionCO, keepAliveInterval, sockOpts, obj);
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
	return ((ClientDataEx*)this)->opCount.GetOpCount();
}


ClientDataEx::ClientDataEx(TCPServ& serv, Socket pc, sfunc func, UINT arrayIndex)
	:
	ClientData(pc, func),
	serv(serv),
	ol(OpType::recv),
	arrayIndex(arrayIndex),
	opCount(),
	olPool(sizeof(OverlappedSendSingle), serv.SingleOlPCCount()),
	opsPending(),
	state(running)
{
	MemPool<HeapAllocator>& pool = serv.GetRecvBuffPool();
	const UINT maxDataSize = serv.GetBufferOptions().GetMaxDataSize();
	char* temp = pool.alloc<char>();
	buff.Initialize(maxDataSize, temp);
}
ClientDataEx::ClientDataEx(ClientDataEx&& clint)
	:
	ClientData(std::forward<ClientData>(*this)),
	serv(clint.serv),
	buff(clint.buff),
	ol(clint.ol),
	arrayIndex(clint.arrayIndex),
	opCount(clint.opCount),
	olPool(std::move(clint.olPool)),
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
		buff = data.buff;
		arrayIndex = data.arrayIndex;
		ol = data.ol;
		opCount = data.opCount;
		olPool = std::move(data.olPool);
		opsPending = std::move(data.opsPending);
		state = data.state;

		memset(&data, 0, sizeof(ClientDataEx));
	}

	return *this;
}
ClientDataEx::~ClientDataEx()
{
	serv.GetRecvBuffPool().dealloc(buff.head);
}


HostSocket::HostSocket(TCPServ& serv)
	:
	serv(serv),
	buffer(),
	ol(OpType::accept)
{}
HostSocket::HostSocket(HostSocket&& host)
	:
	serv(host.serv),
	listen(std::move(host.listen)),
	accept(host.accept),
	buffer(host.buffer),
	ol(host.ol)
{
	memset(&host, 0, sizeof(HostSocket));
}
HostSocket& HostSocket::operator=(HostSocket&& data)
{
	if (this != &data)
	{
		listen = std::move(data.listen);
		accept = data.accept;
		buffer = data.buffer;
		ol = data.ol;

		memset(&data, 0, sizeof(HostSocket));
	}
	return *this;
}
HostSocket::~HostSocket()
{
	dealloc(buffer); //only deallocs if was allocated
}

void HostSocket::SetAcceptSocket()
{
	accept = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
}
void HostSocket::CloseAcceptSocket()
{
	closesocket(accept);
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
					cd.serv.RecvDataCR(bytesTrans, cd);
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
					HostSocket& hostSocket = *(HostSocket*)key;
					hostSocket.serv.AcceptConCR(hostSocket, ol);
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
					cd->serv.RemoveClient(cd, cd->state != ClientDataEx::closing); //closing flag only set when you initiated close
				}
				else
				{
					HostSocket& hostSocket = *(HostSocket*)key;
					hostSocket.serv.CleanupAcceptEx(hostSocket);
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

char* TCPServ::GetSendBuffer()
{
	return bufSendAlloc.GetSendBuffer();
}
MsgStreamWriter TCPServ::CreateOutStream(short type, short msg)
{
	return bufSendAlloc.CreateOutStream(type, msg);
}
const BufferOptions TCPServ::GetBufferOptions() const
{
	return bufSendAlloc.GetBufferOptions();
}

bool TCPServ::SendClientData(const char* data, DWORD nBytes, ClientData* exClient, bool single, CompressionType compType)
{
	return SendClientData(data, nBytes, (ClientDataEx*)exClient, single, false, compType);
}
bool TCPServ::SendClientData(const char* data, DWORD nBytes, ClientData** clients, UINT nClients, CompressionType compType)
{
	return SendClientData(data, nBytes, (ClientDataEx**)clients, nClients, false, compType);
}
bool TCPServ::SendClientData(const char* data, DWORD nBytes, std::vector<ClientData*>& pcs, CompressionType compType)
{
	return SendClientData(data, nBytes, pcs.data(), pcs.size(), compType);
}

bool TCPServ::SendClientData(MsgStreamWriter streamWriter, ClientData* exClient, bool single, CompressionType compType)
{
	return SendClientData(streamWriter, streamWriter.GetSize(), (ClientDataEx*)exClient, single, false, compType);
}
bool TCPServ::SendClientData(MsgStreamWriter streamWriter, ClientData** clients, UINT nClients, CompressionType compType)
{
	return SendClientData(streamWriter, streamWriter.GetSize(), (ClientDataEx**)clients, nClients, false, compType);
}
bool TCPServ::SendClientData(MsgStreamWriter streamWriter, std::vector<ClientData*>& pcs, CompressionType compType)
{
	return SendClientData(streamWriter, streamWriter.GetSize(), pcs.data(), pcs.size(), compType);
}

bool TCPServ::SendClientData(const char* data, DWORD nBytes, ClientDataEx* exClient, bool single, bool msg, CompressionType compType)
{
	return SendClientData(bufSendAlloc.CreateBuff(nBytes, (char*)data, msg, compType), exClient, single);
}
bool TCPServ::SendClientData(const char* data, DWORD nBytes, ClientDataEx** clients, UINT nClients, bool msg, CompressionType compType)
{
	return SendClientData(bufSendAlloc.CreateBuff(nBytes, (char*)data, msg, compType), clients, nClients);
}

bool TCPServ::SendClientSingle(ClientDataEx& clint, OverlappedSendSingle* ol, bool popQueue)
{
	if (clint.pc.IsConnected())
	{
		if (clint.opCount.GetOpCount() < maxPCSendOps)
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
				clint.opsPending.pop();
			}
		}
		else
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

		OverlappedSendSingle* ol = nullptr;
		if (exClient->olPool.IsNotFull())
			ol = exClient->olPool.construct<OverlappedSendSingle>(sendBuff);

		return SendClientSingle(*exClient, ol);
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

		opCounter += nClients;
		for (UINT i = 0; i < nClients; i++)
		{
			ClientDataEx* clint = clients[i];

			ol[i].Initalize(sendInfo);
			if (clint != exClient)
			{
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
		OverlappedSendSingle* ol = nullptr;
		if (clint.olPool.IsNotFull())
			ol = clint.olPool.construct<OverlappedSendSingle>(sendBuff);

		return SendClientSingle(clint, ol);
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

	SendClientData((const char*)msg, MSG_OFFSET, (ClientDataEx*)exClient, single, true, NOCOMPRESSION);
}
void TCPServ::SendMsg(ClientData** clients, UINT nClients, short type, short message)
{
	short msg[] = { type, message };

	SendClientData((const char*)msg, MSG_OFFSET, (ClientDataEx**)clients, nClients, true, NOCOMPRESSION);
}
void TCPServ::SendMsg(std::vector<ClientData*>& pcs, short type, short message)
{
	SendMsg(pcs.data(), (USHORT)pcs.size(), type, message);
}
void TCPServ::SendMsg(const std::tstring& user, short type, short message)
{
	SendMsg((ClientDataEx*)FindClient(user), true, type, message);
}

void TCPServ::RecvDataCR(DWORD bytesTrans, ClientDataEx& cd)
{
	char* ptr = cd.buff.head;

	//Incase there is already a partial block in buffer
	bytesTrans += cd.buff.curBytes;
	cd.buff.curBytes = 0;

	while (true)
	{
		BufSize bufSize(*(DWORD64*)ptr);
		const DWORD bytesToRecv = ((bufSize.up.nBytesComp) ? bufSize.up.nBytesComp : bufSize.up.nBytesDecomp);

		//If there is a full data block ready for processing
		if (bytesTrans - sizeof(DWORD64) >= bytesToRecv)
		{
			const DWORD temp = bytesToRecv + sizeof(DWORD64);
			cd.buff.curBytes += temp;
			bytesTrans -= temp;
			ptr += sizeof(DWORD64);

			//If data was compressed
			if (bufSize.up.nBytesComp)
			{
				BYTE* dest = (BYTE*)(cd.buff.head + sizeof(DWORD64) + GetBufferOptions().GetMaxCompSize());
				if (FileMisc::Decompress(dest, GetBufferOptions().GetMaxCompSize(), (const BYTE*)ptr, bytesToRecv) != UINT_MAX)	// Decompress data
					(cd.func)(cd.serv, &cd, MsgStreamReader{ (char*)dest, bufSize.up.nBytesDecomp - MSG_OFFSET });
				else
					RemoveClient(&cd, cd.state != ClientDataEx::closing);  //Disconnect client because decompress failing is an unrecoverable error
			}
			//If data was not compressed
			else
			{
				(cd.func)(cd.serv, &cd, MsgStreamReader{ (char*)ptr, bufSize.up.nBytesDecomp - MSG_OFFSET });
			}
			//If no partial blocks to copy to start of buffer
			if (!bytesTrans)
			{
				cd.buff.buf = cd.buff.head;
				cd.buff.curBytes = 0;
				break;
			}
		}
		//If the next block of data has not been fully received
		else if (bytesToRecv <= GetBufferOptions().GetMaxDataSize())
		{
			//Concatenate remaining data to buffer
			const DWORD bytesReceived = bytesTrans - cd.buff.curBytes;
			if (cd.buff.head != ptr)
				memcpy(cd.buff.head, ptr, bytesReceived);
			cd.buff.curBytes = bytesReceived;
			cd.buff.buf = cd.buff.head + bytesReceived;
			break;
		}
		else
		{
			assert(false);
			//error has occured
			//To do: have client ask for maximum transfer size
			break;
		}
		ptr += bytesToRecv;
	}

	cd.pc.ReadDataOl(&cd.buff, &cd.ol);
}
void TCPServ::AcceptConCR(HostSocket& host, OverlappedExt* ol)
{
	sockaddr *localAddr, *remoteAddr;
	int localLen, remoteLen;

	//Separate addresses and create new socket
	Socket::GetAcceptExAddrs(host.buffer, host.localAddrLen, host.remoteAddrLen, &localAddr, &localLen, &remoteAddr, &remoteLen);
	Socket socket = host.accept;
	socket.SetAddrInfo(remoteAddr, false);

	//Get ready for another call
	host.SetAcceptSocket();

	//Queue another acceptol
	host.listen.AcceptOl(host.accept, host.buffer, host.localAddrLen, host.remoteAddrLen, ol);

	//If server is not full and has passed the connection condition test
	if (!MaxClients() && connectionCondition(*this, socket))
		host.serv.AddClient(socket); //Add client to server list
	else
		socket.Disconnect();
}
void TCPServ::SendDataCR(ClientDataEx& cd, OverlappedSend* ol)
{
	OverlappedSendInfo* sendInfo = ol->sendInfo;
	if (sendInfo->DecrementRefCount())
		FreeSendOlInfo(sendInfo);

	if ((--opCounter).GetOpCount() == 0)
		SetEvent(shutdownEv);
}	
void TCPServ::SendDataSingleCR(ClientDataEx& cd, OverlappedSendSingle* ol)
{
	FreeSendOlSingle(cd, ol);

	//If there are per client operations pending then attempt to complete them
	if (!cd.opsPending.empty() && cd.state != cd.closing)
		SendClientSingle(cd, cd.opsPending.front(), true);

	if ((--opCounter).GetOpCount() == 0)
		SetEvent(shutdownEv);
}
void TCPServ::CleanupAcceptEx(HostSocket& host)
{
	//Cleanup host and accept sockets
	host.listen.Disconnect();
	host.CloseAcceptSocket();

	if ((--opCounter).GetOpCount() == 0)
		SetEvent(shutdownEv);
}


TCPServ::TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads, DWORD nConcThreads, UINT maxPCSendOps, UINT maxDataSize, UINT singleOlPCCount, UINT allOlCount, UINT sendBuffCount, UINT sendMsgBuffCount, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, SocketOptions sockOpts, void* obj)
	:
	ipv4Host(*this),
	ipv6Host(*this),
	clients(nullptr),
	nClients(0),
	iocp(nThreads, nConcThreads, IOCPThread),
	function(func),
	conFunc(conFunc),
	connectionCondition(nullptr),
	disFunc(disFunc),
	singleOlPCCount(singleOlPCCount),
	maxPCSendOps(maxPCSendOps),
	maxCon(maxCon),
	keepAliveInterval(keepAliveInterval),
	keepAliveHandler(nullptr),
	bufSendAlloc(maxDataSize, sendBuffCount, sendMsgBuffCount, compression, compressionCO),
	clientPool(sizeof(ClientDataEx), maxCon),
	recvBuffPool(sizeof(DWORD64) + maxDataSize * 2, maxCon), //only need maxDataSize * 2 because if compressed data size is > than non compressed, it sends noncompressed
	sendOlInfoPool(sizeof(OverlappedSendInfo), allOlCount),
	sendOlPoolAll(sizeof(OverlappedSend) * maxCon, allOlCount),
	opCounter(),
	shutdownEv(NULL),
	sockOpts(sockOpts),
	obj(obj)
{}
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
	clientSect(serv.clientSect),
	singleOlPCCount(serv.singleOlPCCount),
	maxPCSendOps(serv.maxPCSendOps),
	maxCon(serv.maxCon),
	keepAliveHandler(std::move(serv.keepAliveHandler)),
	bufSendAlloc(std::move(serv.bufSendAlloc)),
	clientPool(std::move(serv.clientPool)),
	recvBuffPool(std::move(serv.recvBuffPool)),
	sendOlInfoPool(std::move(serv.sendOlInfoPool)),
	sendOlPoolAll(std::move(serv.sendOlPoolAll)),
	opCounter(serv.opCounter),
	shutdownEv(serv.shutdownEv),
	sockOpts(serv.sockOpts),
	obj(serv.obj)
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
		clientSect = serv.clientSect;
		const_cast<UINT&>(singleOlPCCount) = serv.singleOlPCCount;
		const_cast<UINT&>(maxPCSendOps) = serv.maxPCSendOps;
		const_cast<UINT&>(maxCon) = serv.maxCon;
		keepAliveHandler = std::move(serv.keepAliveHandler);
		bufSendAlloc = std::move(serv.bufSendAlloc);
		clientPool = std::move(serv.clientPool);
		recvBuffPool = std::move(serv.recvBuffPool);
		sendOlInfoPool = std::move(serv.sendOlInfoPool);
		sendOlPoolAll = std::move(serv.sendOlPoolAll);
		opCounter = serv.opCounter;
		shutdownEv = serv.shutdownEv;
		sockOpts = serv.sockOpts;
		obj = serv.obj;

		ZeroMemory(&serv, sizeof(TCPServ));
	}
	return *this;
}

TCPServ::~TCPServ()
{
	Shutdown();
}

bool TCPServ::BindHost(HostSocket& host, bool ipv6, const LIB_TCHAR* port)
{
	bool b = true;
	if (b = host.listen.Bind(port, ipv6))
	{
		++opCounter; //For AcceptEx
		host.SetAcceptSocket();
		host.buffer = alloc<char>(host.localAddrLen + host.remoteAddrLen);
		if (b = iocp.LinkHandle((HANDLE)host.listen.GetSocket(), &host))
		{
			host.listen.AcceptOl(host.accept, host.buffer, host.localAddrLen, host.remoteAddrLen, &host.ol);
			b = WSAGetLastError() == WSA_IO_PENDING;
		}
	}

	return b;
}
IPv TCPServ::AllowConnections(const LIB_TCHAR* port, ConCondition connectionCondition, IPv ipv)
{
	if(ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected())
		return ipvnone;

	if(!clients)
		clients = alloc<ClientDataEx*>(maxCon);

	this->connectionCondition = connectionCondition;

	if (ipv & ipv4 && !BindHost(ipv4Host, false, port))
		ipv = (IPv)(ipv ^ ipv4);

	if (ipv & ipv6 && !BindHost(ipv6Host, true, port))
		ipv = (IPv)(ipv ^ ipv6);
	
	if (keepAliveInterval != 0.0f)
	{
		keepAliveHandler = construct<KeepAliveHandler>(this);

		if (!keepAliveHandler)
			return ipvnone;
	}

	keepAliveHandler->SetKeepAliveTimer(keepAliveInterval);

	InitializeCriticalSection(&clientSect);

	shutdownEv = CreateEvent(NULL, TRUE, FALSE, NULL);

	return ipv;
}

void TCPServ::AddClient(Socket pc)
{
	++opCounter; //For recv

	EnterCriticalSection(&clientSect);

	ClientDataEx* cd = clients[nClients] = clientPool.construct<ClientDataEx>(*this, pc, function, nClients);
	nClients += 1;

	LeaveCriticalSection(&clientSect);

	if (sockOpts.NoDelay())
		pc.SetNoDelay(true);
	if (sockOpts.UseOwnBuf())
		pc.SetTCPSendStack();

	iocp.LinkHandle((HANDLE)pc.GetSocket(), cd);

	pc.ReadDataOl(&cd->buff, &cd->ol);

	RunConFunc(cd);
}
void TCPServ::RemoveClient(ClientDataEx* client, bool unexpected)
{
	//Cleanup all pending operations
	for (int i = 0, size = client->opsPending.size(); i < size; i++)
	{
		SendDataSingleCR(*client, client->opsPending.front());
		client->opsPending.pop();
	}

	RunDisFunc(client, unexpected);

	EnterCriticalSection(&clientSect);

	const UINT index = client->arrayIndex;
	ClientDataEx*& data = clients[index];

	data->pc.Disconnect();
	clientPool.destruct(data);

	if(index != (nClients - 1))
	{
		data = clients[nClients - 1];
		data->arrayIndex = index;
	}

	nClients -= 1;

	LeaveCriticalSection(&clientSect);

	if ((--opCounter).GetOpCount() == 0)
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
		//Stop sending keepalive packets
		destruct(keepAliveHandler);

		//Stop accepting new connections
		ipv4Host.listen.Disconnect();
		ipv6Host.listen.Disconnect();

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
		iocp.Post(0, nullptr);

		//Wait for iocp threads to close, then cleanup iocp
		iocp.WaitAndCleanup();

		//Free up client array
		dealloc(clients);

		DeleteCriticalSection(&clientSect);

		CloseHandle(shutdownEv);
		shutdownEv = NULL;
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

Socket TCPServ::GetHostIPv4() const
{
	return ipv4Host.listen;
}
Socket TCPServ::GetHostIPv6() const
{
	return ipv6Host.listen;
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
	conFunc(client);
}
void TCPServ::RunDisFunc(ClientData* client, bool unexpected)
{
	disFunc(client, unexpected);
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
	return ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected();
}

const SocketOptions TCPServ::GetSockOpts() const
{
	return sockOpts;
}

UINT TCPServ::GetOpCount() const
{
	return opCounter.GetOpCount();
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
	return iocp;
}

MemPool<HeapAllocator>& TCPServ::GetRecvBuffPool()
{
	return recvBuffPool;
}