#include "StdAfx.h"
#include "Mouse.h"

MouseClient::MouseClient( MouseServer& server )
	:
	server( server )
{}

MouseEvent MouseClient::Read()
{
	if(!server.buffer.empty())
	{
		const MouseEvent e = server.buffer.front();
		server.buffer.pop_front();
		return e;
	}
	else
	{
		return MouseEvent(MouseEvent::Invalid, 0, 0);
	}
}
MouseEvent MouseClient::Peek() const
{
	if (!server.buffer.empty())
		return server.buffer.front();
	else
		return MouseEvent(MouseEvent::Invalid, 0, 0);
}

bool MouseClient::MouseEmpty() const
{
	return server.buffer.empty( );
}

size_t MouseClient::EventCount() const
{
	return server.buffer.size();
}
const MouseEvent& MouseClient::GetEvent(size_t i)
{
	return server.buffer[i];
}

void MouseClient::Erase(size_t count)
{
	server.buffer.erase(count);
}


MouseServer::MouseServer(size_t bufferSize, USHORT interval)
	:
	buffer(bufferSize),
	interval(interval)
{}
MouseServer::MouseServer(MouseServer&& mServ)
	:
	buffer(std::move(mServ.buffer)),
	interval(mServ.interval)
{
	ZeroMemory(&mServ, sizeof(MouseServer));
}

void MouseServer::OnMouseMove(USHORT x, USHORT y)
{
	WaitForBuffer();
	buffer.push_back(MouseEvent(MouseEvent::Move, x, y));
}
void MouseServer::OnLeftPressed( USHORT x,USHORT y )
{
	WaitForBuffer();
	buffer.push_back(MouseEvent(MouseEvent::LPress, x, y));
}
void MouseServer::OnLeftReleased( USHORT x, USHORT y )
{
	WaitForBuffer();
	buffer.push_back(MouseEvent(MouseEvent::LRelease, x, y));
}
void MouseServer::OnRightPressed( USHORT x,USHORT y )
{
	WaitForBuffer();
	buffer.push_back(MouseEvent(MouseEvent::RPress, x, y));
}
void MouseServer::OnRightReleased( USHORT x,USHORT y )
{
	WaitForBuffer();
	buffer.push_back(MouseEvent(MouseEvent::RRelease, x, y));
}
void MouseServer::OnWheelUp( USHORT x,USHORT y )
{
	WaitForBuffer();
	buffer.push_back(MouseEvent(MouseEvent::WheelUp, x, y));
}
void MouseServer::OnWheelDown( USHORT x,USHORT y )
{
	WaitForBuffer();
	buffer.push_back(MouseEvent(MouseEvent::WheelDown, x, y));
}

UINT MouseServer::GetBufferLen(UINT& count) const
{
	count = buffer.size();
	return count * sizeof(MouseEvent);
}
void MouseServer::Extract(BYTE *byteBuffer, UINT count)
{
	for(UINT i = 0; i < count; i++)
		((MouseEvent*)(byteBuffer))[i] = buffer[i];

	buffer.erase(count);
}
void MouseServer::Insert(BYTE *byteBuffer, DWORD nBytes)
{
	const size_t count = nBytes / sizeof(MouseEvent);
	while((int)buffer.max_size() - (int)(buffer.size() + count) < 0)
		Sleep(interval);

	for(UINT i = 0; i < count; i++)
		buffer.push_back_ninc(*(((MouseEvent*)byteBuffer) + i));

	buffer.IncreaseWritten(count);
}

void MouseServer::WaitForBuffer()
{
	while(buffer.size() == buffer.max_size())
	{
		Sleep(interval);
	}
}
