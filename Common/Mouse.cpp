#include "Mouse.h"
#include "HeapAlloc.h"

MouseClient::MouseClient( MouseServer& server )
: server( server )
{}
USHORT MouseClient::GetX() const
{
	return server.x;
}
USHORT MouseClient::GetY() const
{
	return server.y;
}
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
	if( server.buffer.size() > 0 )
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
	if (server.buffer.size() > 0)
	{
		MouseEvent e = server.buffer.front();
		return e;
	}
	else
	{
		return MouseEvent(MouseEvent::Invalid, 0, 0);
	}
}
bool MouseClient::MouseEmpty( ) const
{
	return server.buffer.empty( );
}


MouseServer::MouseServer()
:	isInWindow( false ),
	leftIsPressed( false ),
	rightIsPressed( false ),
	x( -1 ),
	y( -1 )
{}
void MouseServer::OnMouseMove( USHORT x,USHORT y )
{
	this->x = x;
	this->y = y;

	buffer.push( MouseEvent( MouseEvent::Move,x,y ) );
	if( buffer.size( ) > bufferSize )
	{
		buffer.pop( );
	}
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
	if( buffer.size( ) > bufferSize )
	{
		buffer.pop( );
	}
}
void MouseServer::OnLeftReleased( USHORT x,USHORT y )
{
	leftIsPressed = false;

	buffer.push( MouseEvent( MouseEvent::LRelease,x,y ) );
	if( buffer.size( ) > bufferSize )
	{
		buffer.pop( );
	}
}
void MouseServer::OnRightPressed( USHORT x,USHORT y )
{
	rightIsPressed = true;

	buffer.push( MouseEvent( MouseEvent::RPress,x,y ) );
	if( buffer.size( ) > bufferSize )
	{
		buffer.pop( );
	}
}
void MouseServer::OnRightReleased( USHORT x,USHORT y )
{
	rightIsPressed = false;

	buffer.push( MouseEvent( MouseEvent::RRelease,x,y ) );
	if( buffer.size( ) > bufferSize )
	{
		buffer.pop( );
	}
}
void MouseServer::OnWheelUp( USHORT x,USHORT y )
{
	buffer.push( MouseEvent( MouseEvent::WheelUp,x,y ) );
	if( buffer.size( ) > bufferSize )
	{
		buffer.pop( );
	}

}
void MouseServer::OnWheelDown( USHORT x,USHORT y )
{
	buffer.push( MouseEvent( MouseEvent::WheelDown,x,y ) );
	if( buffer.size( ) > bufferSize )
	{
		buffer.pop( );
	}

}
bool MouseServer::IsInWindow() const
{
	return isInWindow;
}

UINT MouseServer::GetBufferLen() const
{
	return buffer.size() * sizeof(MouseEvent::Type);
}

void MouseServer::Extract(BYTE *byteBuffer)
{
	MouseServer *serv = (MouseServer*)byteBuffer;
	serv->x = x;
	serv->y = y;
	serv->leftIsPressed = leftIsPressed;
	serv->rightIsPressed = rightIsPressed;
	serv->isInWindow = isInWindow;
	const UINT offsetToBuffer = sizeof(MouseServer) - sizeof(std::deque<MouseEvent>);

	for (unsigned int i = 0; i < buffer.size(); i++)
	{
		MouseEvent *mEvent = (MouseEvent*)&byteBuffer[offsetToBuffer];
		mEvent[i] = buffer.front();
		buffer.pop();
	}
}
