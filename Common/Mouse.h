#pragma once
#include "CircularBuffer.h"

class MouseServer;

typedef unsigned long DWORD;
typedef unsigned short USHORT;
typedef unsigned char BYTE;
typedef unsigned int UINT;

class MouseEvent
{
public:
	enum Type
	{
		LPress,
		LRelease,
		RPress,
		RRelease,
		WheelUp,
		WheelDown,
		Move,
		Invalid
	};
private:
	Type type;
	USHORT x, y;
public:
	MouseEvent()
		:
		type(Invalid),
		x(USHRT_MAX),
		y(USHRT_MAX)
	{}

	MouseEvent(Type type, USHORT x, USHORT y)
		:
		type( type ),
		x( x ),
		y( y )
	{}
	MouseEvent& operator=(const MouseEvent& mevt)
	{
		type = mevt.type;
		x = mevt.x;
		y = mevt.y;
		return *this;
	}

	bool IsValid() const
	{
		return type != Invalid;
	}
	Type GetType() const
	{
		return type;
	}
	USHORT GetX() const
	{
		return x;
	}
	USHORT GetY() const
	{
		return y;
	}
};

class MouseClient
{
public:
	MouseClient(MouseServer& server);
	bool IsInWindow() const;
	MouseEvent Read();
	MouseEvent Peek() const;
	size_t EventCount() const;
	const MouseEvent& GetEvent(size_t i);
	void Erase(size_t count);
	bool MouseEmpty() const;
private:
	MouseServer& server;
};

class MouseServer
{
	friend MouseClient;
public:
	MouseServer(size_t bufferSize, USHORT interval);
	MouseServer(MouseServer&& mServ);
	void OnMouseMove(USHORT x, USHORT y);
	void OnLeftPressed(USHORT x, USHORT y);
	void OnLeftReleased(USHORT x, USHORT y);
	void OnRightPressed(USHORT x, USHORT y);
	void OnRightReleased(USHORT x, USHORT y);
	void OnWheelUp(USHORT x, USHORT y);
	void OnWheelDown(USHORT x, USHORT y);
	bool IsInWindow() const;
	void Extract(BYTE *byteBuffer, UINT count);
	void Insert(BYTE *byteBuffer, DWORD nBytes);
	UINT GetBufferLen(UINT& count) const;
private:
	void WaitForBuffer();

	CircularBuffer<MouseEvent> buffer;
	USHORT interval;
};