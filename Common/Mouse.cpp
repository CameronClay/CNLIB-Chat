#include "Mouse.h"
#include "CNLIB\HeapAlloc.h"

MouseClient::MouseClient( MouseServer& server )
: server( server )
{}
bool MouseClient::LeftIsPressed() const
{
	return server.leftIsPressed;
}
bool MouseClient::RightIsPressed() const
{
	return server.rightIsPressed;
}
bool MouseClient::IsInWindow() const
{
	return server.isInWindow;
}
MouseEvent MouseClient::Read()
{
	if(!server.buffer.empty())
	{
		MouseEvent e = server.buffer.front( );
		server.buffer.pop( );
		return e;
	}
	else
	{
		return MouseEvent( MouseEvent::Invalid,0,0 );
	}
}
MouseEvent MouseClient::Peek()
{
	if (!server.buffer.empty())
		return server.buffer.front();
	else
		return MouseEvent(MouseEvent::Invalid, 0, 0);
}
bool MouseClient::MouseEmpty( ) const
{
	return server.buffer.empty( );
}


MouseServer::MouseServer()
:	isInWindow( false ),
	leftIsPressed( false ),
	rightIsPressed( false )
{}
void MouseServer::OnMouseMove(USHORT x, USHORT y)
{
	if(x > 800)
	{
		int a = 0;
	}
	if(y > 600)
	{
		int a = 0;
	}
	if(buffer.size() >= 5)
	{
		int a = 0;
	}
	buffer.push(MouseEvent(MouseEvent::Move, x, y));
}
void MouseServer::OnMouseLeave()
{
	isInWindow = false;
}
void MouseServer::OnMouseEnter()
{
	isInWindow = true;
}
void MouseServer::OnLeftPressed( USHORT x,USHORT y )
{
	leftIsPressed = true;
	buffer.push( MouseEvent( MouseEvent::LPress,x,y ) );
}
void MouseServer::OnLeftReleased( USHORT x, USHORT y )
{
	leftIsPressed = false;
	buffer.push( MouseEvent( MouseEvent::LRelease,x,y ) );
}
void MouseServer::OnRightPressed( USHORT x,USHORT y )
{
	rightIsPressed = true;
	buffer.push( MouseEvent( MouseEvent::RPress,x,y ) );
}
void MouseServer::OnRightReleased( USHORT x,USHORT y )
{
	rightIsPressed = false;
	buffer.push( MouseEvent( MouseEvent::RRelease,x,y ) );
}
void MouseServer::OnWheelUp( USHORT x,USHORT y )
{
	buffer.push( MouseEvent( MouseEvent::WheelUp,x,y ) );
}
void MouseServer::OnWheelDown( USHORT x,USHORT y )
{
	buffer.push( MouseEvent( MouseEvent::WheelDown,x,y ) );
}
bool MouseServer::IsInWindow() const
{
	return isInWindow;
}

UINT MouseServer::GetBufferLen(UINT& count) const
{
	count = buffer.size();
	return (sizeof(bool) * 3)/* + (sizeof(USHORT) * 2)*/ + (count * sizeof(MouseEvent));
}

void MouseServer::Extract(BYTE *byteBuffer, UINT count)
{
	UINT pos = 0;

	*(bool*)&byteBuffer[pos] = leftIsPressed;
	pos += sizeof(bool);
	*(bool*)&byteBuffer[pos] = rightIsPressed;
	pos += sizeof(bool);
	*(bool*)&byteBuffer[pos] = isInWindow;
	pos += sizeof(bool);

	for(UINT i = 0; i < count; i++)
	{
		*(MouseEvent*)&byteBuffer[pos] = buffer.front();
		pos += sizeof(MouseEvent);
		buffer.pop();
	}
}

void MouseServer::Insert(BYTE *byteBuffer, DWORD nBytes)
{
	UINT pos = 0;

	leftIsPressed = *(bool*)&byteBuffer[pos];
	pos += sizeof(bool);
	rightIsPressed = *(bool*)&byteBuffer[pos];
	pos += sizeof(bool);
	isInWindow = *(bool*)&byteBuffer[pos];
	pos += sizeof(bool);


	while(pos != nBytes)
	{
		buffer.push(*(MouseEvent*)&byteBuffer[pos]);
		pos += sizeof(MouseEvent);
	}
}
