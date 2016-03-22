#include "StdAfx.h"
#include "TCPServ.h"
#include "CNLIB/BufSize.h"
#include "CNLIB/File.h"
#include "CNLIB/Messages.h"
#include "CNLIB/MsgHeader.h"

TCPServInterface* CreateServer(sfunc msgHandler, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads, DWORD nConcThreads, UINT maxPCSendOps, UINT maxDataBuffSize, UINT singleOlPCCount, UINT allOlCount, UINT sendBuffCount, UINT sendCompBuffCount, UINT sendMsgBuffCount, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, SocketOptions sockOpts, void* obj)
{
	return construct<TCPServ>(msgHandler, conFunc, disFunc, nThreads, nConcThreads, maxPCSendOps, maxDataBuffSize, singleOlPCCount, allOlCount, sendBuffCount, sendCompBuffCount, sendMsgBuffCount, maxCon, compression, compressionCO, keepAliveInterval, sockOpts, obj);
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
	recvHandler(serv.GetBufferOptions(), 2, &serv),
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
ClientDataEx::~ClientDataEx()
{}

bool ClientDataEx::DecRefCount()
{
	return --opCount == 0;
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
	bool b = true;
	for (UINT i = 0; i < nConcAccepts; i++)
		b &= iocp.LinkHandle((HANDLE)listen.GetSocket(), &acceptData[i]);
	return b;
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
	ol(OpType::accept)
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

					//Access violation problem
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
					HostSocket::AcceptData& acceptData = *(HostSocket::AcceptData*)key;
					acceptData.AcceptConCR();
					/*HostSocket& hostSocket = *(HostSocket*)key;
					hostSocket.serv.AcceptConCR(hostSocket, ol);*/
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
					//Access violation problem
					ClientDataEx* cd = (ClientDataEx*)key;
					if (cd->DecRefCount())
						cd->serv.RemoveClient(cd, cd->state != ClientDataEx::closing); //closing flag only set when you initiated close
				}
				else
				{
					HostSocket::AcceptData& acceptData = *(HostSocket::AcceptData*)key;
					acceptData.GetHostSocket()->GetServ().CleanupAcceptEx(acceptData);
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

bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientData* exClient, bool single, BuffAllocator* alloc)
{
	return SendClientData(buffSendInfo, nBytes, (ClientDataEx*)exClient, single, false, alloc);
}
bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientData** clients, UINT nClients, BuffAllocator* alloc)
{
	return SendClientData(buffSendInfo, nBytes, (ClientDataEx**)clients, nClients, false, alloc);
}
bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, std::vector<ClientData*>& pcs, BuffAllocator* alloc)
{
	return SendClientData(buffSendInfo, nBytes, pcs.data(), pcs.size(), alloc);
}

bool TCPServ::SendClientData(const MsgStreamWriter& streamWriter, ClientData* exClient, bool single, BuffAllocator* alloc)
{
	return SendClientData(streamWriter.GetBuffSendInfo(), streamWriter.GetSize(), (ClientDataEx*)exClient, single, false, alloc);
}
bool TCPServ::SendClientData(const MsgStreamWriter& streamWriter, ClientData** clients, UINT nClients, BuffAllocator* alloc)
{
	return SendClientData(streamWriter.GetBuffSendInfo(), streamWriter.GetSize(), (ClientDataEx**)clients, nClients, false, alloc);
}
bool TCPServ::SendClientData(const MsgStreamWriter& streamWriter, std::vector<ClientData*>& pcs, BuffAllocator* alloc)
{
	return SendClientData(streamWriter.GetBuffSendInfo(), streamWriter.GetSize(), pcs.data(), pcs.size(), alloc);
}

bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientDataEx* exClient, bool single, bool msg, BuffAllocator* alloc)
{
	return SendClientData(bufSendAlloc.CreateBuff(buffSendInfo, nBytes, msg, -1, alloc), exClient, single);
}
bool TCPServ::SendClientData(const BuffSendInfo& buffSendInfo, DWORD nBytes, ClientDataEx** clients, UINT nClients, bool msg, BuffAllocator* alloc)
{
	return SendClientData(bufSendAlloc.CreateBuff(buffSendInfo, nBytes, msg, -1, alloc), clients, nClients);
}

bool TCPServ::SendClientSingle(ClientDataEx& clint, OverlappedSendSingle* ol, bool popQueue)
{
	if (clint.pc.IsConnected())
	{
		if (clint.opCount.load() < maxPCSendOps)
		{
			++clint.opCount;
			++opCounter;

			//UINT localOpIndex = ++clint.opIndex;
			//ol->sendBuff.SetOpIndex(opIndex.GetCount() + localOpIndex);

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

//void TCPServ::RecvDataCR(DWORD bytesTrans, ClientDataEx& cd)
//{
//	char* ptr = cd.buff.head;
//
//	//Incase there is already a partial block in buffer
//	bytesTrans += cd.buff.curBytes;
//	cd.buff.curBytes = 0;
//
//	while (true)
//	{
//		DataHeader& header = *(DataHeader*)ptr;
//		const DWORD bytesToRecv = ((header.size.up.nBytesComp) ? header.size.up.nBytesComp : header.size.up.nBytesDecomp);
//
//		//If there is a full data block ready for processing
//		if (bytesTrans - sizeof(DWORD64) >= bytesToRecv)
//		{
//			const DWORD temp = bytesToRecv + sizeof(DWORD64);
//			cd.buff.curBytes += temp;
//			bytesTrans -= temp;
//			ptr += sizeof(DWORD64);
//
//			//If data was compressed
//			if (header.size.up.nBytesComp)
//			{
//				BYTE* dest = (BYTE*)(cd.buff.head + sizeof(DWORD64) + GetBufferOptions().GetMaxCompSize());
//				if (FileMisc::Decompress(dest, GetBufferOptions().GetMaxCompSize(), (const BYTE*)ptr, bytesToRecv) != UINT_MAX)	// Decompress data
//					(cd.func)(cd.serv, &cd, MsgStreamReader{ (char*)dest, header.size.up.nBytesDecomp - MSG_OFFSET });
//				else
//					RemoveClient(&cd, cd.state != ClientDataEx::closing);  //Disconnect client because decompress failing is an unrecoverable error
//			}
//			//If data was not compressed
//			else
//			{
//				(cd.func)(cd.serv, &cd, MsgStreamReader{ (char*)ptr, header.size.up.nBytesDecomp - MSG_OFFSET });
//			}
//			//If no partial blocks to copy to start of buffer
//			if (!bytesTrans)
//			{
//				cd.buff.buf = cd.buff.head;
//				cd.buff.curBytes = 0;
//				break;
//			}
//		}
//		//If the next block of data has not been fully received
//		else if (bytesToRecv <= GetBufferOptions().GetMaxDataSize())
//		{
//			//Concatenate remaining data to buffer
//			const DWORD bytesReceived = bytesTrans - cd.buff.curBytes;
//			if (cd.buff.head != ptr)
//				memcpy(cd.buff.head, ptr, bytesReceived);
//			cd.buff.curBytes = bytesReceived;
//			cd.buff.buf = cd.buff.head + bytesReceived;
//			break;
//		}
//		else
//		{
//			assert(false);
//			//error has occured
//			//To do: have client ask for maximum transfer size
//			break;
//		}
//		ptr += bytesToRecv;
//	}
//
//	cd.pc.ReadDataOl(&cd.buff, &cd.ol);
//}
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
		SendClientSingle(cd, cd.opsPending.front(), true);

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


TCPServ::TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads, DWORD nConcThreads, UINT maxPCSendOps, UINT maxDataBuffSize, UINT singleOlPCCount, UINT allOlCount, UINT sendBuffCount, UINT sendCompBuffCount, UINT sendMsgBuffCount, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, SocketOptions sockOpts, void* obj)
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
	bufSendAlloc(maxDataBuffSize, sendBuffCount, sendCompBuffCount, sendMsgBuffCount, compression, compressionCO),
	clientPool(sizeof(ClientDataEx), maxCon),
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
	sendOlInfoPool(std::move(serv.sendOlInfoPool)),
	sendOlPoolAll(std::move(serv.sendOlPoolAll)),
	opCounter(serv.opCounter.load()),
	shutdownEv(serv.shutdownEv),
	sockOpts(serv.sockOpts),
	obj(serv.obj)
{
	int size = sizeof(DataHeader);
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
		sendOlInfoPool = std::move(serv.sendOlInfoPool);
		sendOlPoolAll = std::move(serv.sendOlPoolAll);
		opCounter = serv.opCounter.load();
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

bool TCPServ::BindHost(HostSocket& host, const LIB_TCHAR* port, bool ipv6, UINT nConcAccepts)
{
	bool b = true;

	if (b = host.Bind(port, ipv6))
	{
		host.Initalize(nConcAccepts);
		if (b = host.LinkIOCP(iocp))
		{
			host.QueueAccept();
			if (b = (WSAGetLastError() == ERROR_IO_PENDING))
				opCounter += nConcAccepts;
		}
	}

	return b;
}
IPv TCPServ::AllowConnections(const LIB_TCHAR* port, ConCondition connectionCondition, IPv ipv, UINT nConcAccepts)
{
	if(ipv4Host.GetListen().IsConnected() || ipv6Host.GetListen().IsConnected())
		return ipvnone;

	if(!clients)
		clients = alloc<ClientDataEx*>(maxCon);

	this->connectionCondition = connectionCondition;

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

	cd->recvHandler.StartRead(pc);

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
		iocp.Post(0, nullptr);

		//Wait for iocp threads to close, then cleanup iocp
		iocp.WaitAndCleanup();

		//Free up client array
		dealloc(clients);

		//Cleanup accept data
		ipv4Host.Cleanup();
		ipv6Host.Cleanup();

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
	return ipv4Host.GetListen().IsConnected() || ipv6Host.GetListen().IsConnected();
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
	return iocp;
}