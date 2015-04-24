/****************************************************************************************** 
 *	Chili DirectX Framework Version 14.03.22											  *	
 *	Mouse.h																				  *
 *	Copyright 2014 PlanetChili <http://www.planetchili.net>								  *
 *																						  *
 *	This file is part of The Chili DirectX Framework.									  *
 *																						  *
 *	The Chili DirectX Framework is free software: you can redistribute it and/or modify	  *
 *	it under the terms of the GNU General Public License as published by				  *
 *	the Free Software Foundation, either version 3 of the License, or					  *
 *	(at your option) any later version.													  *
 *																						  *
 *	The Chili DirectX Framework is distributed in the hope that it will be useful,		  *
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
 *	GNU General Public License for more details.										  *
 *																						  *
 *	You should have received a copy of the GNU General Public License					  *
 *	along with The Chili DirectX Framework.  If not, see <http://www.gnu.org/licenses/>.  *
 ******************************************************************************************/
#pragma once
#include <queue>


class MouseServer;

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
	MouseEvent(Type type, USHORT x, USHORT y)
		:
		type( type ),
		x( x ),
		y( y )
	{}
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
	USHORT GetX() const;
	USHORT GetY() const;
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
	UINT Extract(BYTE *byteBuffer);
private:
	USHORT x, y;
	bool leftIsPressed, rightIsPressed, isInWindow;
	static const USHORT bufferSize = 4;
	std::queue< MouseEvent > buffer;
};