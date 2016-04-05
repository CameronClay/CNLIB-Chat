#pragma once
//#include "CircularBuffer.h"
#include "CNLIB/CritLock.h"

class MouseServer;

typedef unsigned long DWORD;
typedef unsigned short USHORT;
typedef unsigned char BYTE;
typedef unsigned int UINT;

class MouseEvent
{
public:
	enum Type : UCHAR
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
		if (this != &mevt)
		{
			type = mevt.type;
			x = mevt.x;
			y = mevt.y;
		}
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
	MouseClient operator=(MouseClient) = delete;

	bool MouseEmpty() const;

	MouseEvent Read();
	MouseEvent Peek() const;

	size_t EventCount() const;
private:
	MouseServer& server;
};

class MouseServer
{
	friend MouseClient;
public:
	MouseServer(USHORT interval);
	MouseServer(MouseServer&& mServ);

	void OnMouseMove(USHORT x, USHORT y);
	void OnLeftPressed(USHORT x, USHORT y);
	void OnLeftReleased(USHORT x, USHORT y);
	void OnRightPressed(USHORT x, USHORT y);
	void OnRightReleased(USHORT x, USHORT y);
	void OnWheelUp(USHORT x, USHORT y);
	void OnWheelDown(USHORT x, USHORT y);

	void Extract(BYTE *byteBuffer, UINT count);
	void Insert(BYTE *byteBuffer, DWORD nBytes);
	UINT GetBufferLen(UINT& count) const;

private:
	//void WaitForBuffer();

	std::queue<MouseEvent> queue;
	CritLock bufferLock;
	//CircularBuffer<MouseEvent> buffer;
	USHORT interval;
};
