#include "Mouse.h"
#include "CNLIB\HeapAlloc.h"

MouseClient::MouseClient( MouseServer& server )
: server( server )
{}
MouseEvent MouseClient::Read()
{
	if(!server.buffer.empty())
	{
		const MouseEvent e = server.buffer.front( );
		server.buffer.pop_front();
		return e;
	}
	else
	{
		return MouseEvent( MouseEvent::Invalid,0,0 );
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


MouseServer::MouseServer(size_t bufferSize)
:	
buffer( bufferSize )
{}
MouseServer::MouseServer(MouseServer&& mServ)
	:
	buffer(std::move(mServ.buffer))
{
	ZeroMemory(&mServ, sizeof(MouseServer));
}

void MouseServer::OnMouseMove(USHORT x, USHORT y)
{
	buffer.push_back(MouseEvent(MouseEvent::Move, x, y));
	if(buffer.size() + 3 > buffer.max_size())
	{
		int a = 0;
	}
}
void MouseServer::OnLeftPressed( USHORT x,USHORT y )
{
	buffer.push_back(MouseEvent(MouseEvent::LPress, x, y));
}
void MouseServer::OnLeftReleased( USHORT x, USHORT y )
{
	buffer.push_back(MouseEvent(MouseEvent::LRelease, x, y));
}
void MouseServer::OnRightPressed( USHORT x,USHORT y )
{
	buffer.push_back(MouseEvent(MouseEvent::RPress, x, y));
}
void MouseServer::OnRightReleased( USHORT x,USHORT y )
{
	buffer.push_back(MouseEvent(MouseEvent::RRelease, x, y));
}
void MouseServer::OnWheelUp( USHORT x,USHORT y )
{
	buffer.push_back(MouseEvent(MouseEvent::WheelUp, x, y));
}
void MouseServer::OnWheelDown( USHORT x,USHORT y )
{
	buffer.push_back(MouseEvent(MouseEvent::WheelDown, x, y));
}

UINT MouseServer::GetBufferLen(UINT& count) const
{
	count = buffer.size();
	return count * sizeof(MouseEvent);
}
void MouseServer::Extract(BYTE *byteBuffer, UINT count)
{
	UINT pos = 0;

	for(UINT i = 0; i < count; i++)
	{
		*(MouseEvent*)&byteBuffer[pos] = buffer.front();
		pos += sizeof(MouseEvent);
		buffer.pop_front();
	}
}
void MouseServer::Insert(BYTE *byteBuffer, DWORD nBytes)
{
	UINT pos = 0;

	while(pos != nBytes)
	{
		buffer.push_back(*(MouseEvent*)&byteBuffer[pos]);
		pos += sizeof(MouseEvent);
	}
}
