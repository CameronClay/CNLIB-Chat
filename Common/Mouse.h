#pragma once
#include <queue>


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
	int x;
	int y;
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
		const_cast<Type>(type) = mevt.type;
		const_cast<int&>(x) = mevt.x;
		const_cast<int&>(y) = mevt.y;
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
	MouseClient( MouseServer& server );
	bool LeftIsPressed() const;
	bool RightIsPressed() const;
	bool IsInWindow() const;
	MouseEvent Read();
	MouseEvent Peek();
	bool MouseEmpty() const;
private:
	MouseServer& server;
};

class MouseServer
{
	friend MouseClient;
public:
	MouseServer();
	void OnMouseMove(USHORT x, USHORT y);
	void OnMouseLeave();
	void OnMouseEnter();
	void OnLeftPressed(USHORT x, USHORT y);
	void OnLeftReleased(USHORT x, USHORT y);
	void OnRightPressed(USHORT x, USHORT y);
	void OnRightReleased(USHORT x, USHORT y);
	void OnWheelUp(USHORT x, USHORT y);
	void OnWheelDown(USHORT x, USHORT y);
	bool IsInWindow() const;
	void Extract(BYTE *byteBuffer);
	void Insert(BYTE *byteBuffer, DWORD nBytes);
	UINT GetBufferLen() const;
private:
	bool leftIsPressed, rightIsPressed, isInWindow;
	std::queue< MouseEvent > buffer;
};