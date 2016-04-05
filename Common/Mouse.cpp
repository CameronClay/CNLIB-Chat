#include "StdAfx.h"
#include "Mouse.h"

MouseClient::MouseClient( MouseServer& server )
	:
	server( server )
{}

MouseEvent MouseClient::Read()
{
	if(!server.queue.empty())
	{
		server.bufferLock.Lock();
		const MouseEvent e = server.queue.front();
		server.queue.pop();
		server.bufferLock.Unlock();
		return e;
	}
	else
	{
		return MouseEvent(MouseEvent::Invalid, 0, 0);
	}
}
MouseEvent MouseClient::Peek() const
{
	if (!server.queue.empty())
		return server.queue.front();
	else
		return MouseEvent(MouseEvent::Invalid, 0, 0);
}

bool MouseClient::MouseEmpty() const
{
	return server.queue.empty();
}

size_t MouseClient::EventCount() const
{
	return server.queue.size();
}


MouseServer::MouseServer(USHORT interval)
	:
	queue(),
	interval(interval)
{
}
MouseServer::MouseServer(MouseServer&& mServ)
	:
	queue(std::move(mServ.queue)),
	interval(mServ.interval)
{
	ZeroMemory(&mServ, sizeof(MouseServer));
}

void MouseServer::OnMouseMove(USHORT x, USHORT y)
{
	bufferLock.Lock();
	queue.emplace(MouseEvent::Move, x, y);
	bufferLock.Unlock();
}
void MouseServer::OnLeftPressed( USHORT x,USHORT y )
{
	bufferLock.Lock();
	queue.emplace(MouseEvent::LPress, x, y);
	bufferLock.Unlock();
}
void MouseServer::OnLeftReleased( USHORT x, USHORT y )
{
	bufferLock.Lock();
	queue.emplace(MouseEvent::LRelease, x, y);
	bufferLock.Unlock();
}
void MouseServer::OnRightPressed( USHORT x,USHORT y )
{
	bufferLock.Lock();
	queue.emplace(MouseEvent::RPress, x, y);
	bufferLock.Unlock();
}
void MouseServer::OnRightReleased( USHORT x,USHORT y )
{
	bufferLock.Lock();
	queue.emplace(MouseEvent::RRelease, x, y);
	bufferLock.Unlock();
}
void MouseServer::OnWheelUp( USHORT x,USHORT y )
{
	bufferLock.Lock();
	queue.emplace(MouseEvent::WheelUp, x, y);
	bufferLock.Unlock();
}
void MouseServer::OnWheelDown( USHORT x,USHORT y )
{
	bufferLock.Lock();
	queue.emplace(MouseEvent::WheelDown, x, y);
	bufferLock.Unlock();
}

UINT MouseServer::GetBufferLen(UINT& count) const
{
	count = queue.size();
	return count * sizeof(MouseEvent);
}
void MouseServer::Extract(BYTE *byteBuffer, UINT count)
{
	bufferLock.Lock();

	for (UINT i = 0; i < count; i++)
	{
		((MouseEvent*)(byteBuffer))[i] = queue.front();
		queue.pop();
	}

	bufferLock.Unlock();
}
void MouseServer::Insert(BYTE *byteBuffer, DWORD nBytes)
{
	bufferLock.Lock();

	for (UINT i = 0, count = nBytes / sizeof(MouseEvent); i < count; i++)
		queue.push(*(((MouseEvent*)byteBuffer) + i));

	bufferLock.Unlock();
}