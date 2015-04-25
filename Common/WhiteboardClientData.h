#pragma once

#include <vector>
#include <Windows.h>

#include "Mouse.h"

enum class Tool
{
	PaintBrush, Line, Rect, FilledRect, ellipse, FilledEllipse
};

struct PointU
{
	PointU()
		:
		x(0),
		y(0)
	{}
	PointU(USHORT X, USHORT Y)
		:
		x(X),
		y(Y)
	{}
	PointU(PointU &&pt) :
		x(pt.x),
		y(pt.y)
	{
		ZeroMemory(&pt, sizeof(PointU));
	}
	PointU operator*(const int val)
	{
		PointU temp;
		temp.x = x * val;
		temp.y = y * val;
		return temp;
	}
	PointU operator+(const PointU &val)
	{
		PointU temp;
		temp.x = x + val.x;
		temp.y = y + val.y;
		return temp;
	}
	PointU operator-(const PointU &val)
	{
		PointU temp;
		temp.x = x - val.x;
		temp.y = y - val.y;
		return temp;
	}
	PointU &operator=(PointU &&val)
	{
		x = val.x;
		y = val.y;
		ZeroMemory(&val, sizeof(PointU));

		return (*this);
	}

	PointU Sqr()
	{
		PointU pt;
		pt.x = x * x;
		pt.y = y * y;
		return pt;
	}
	float Length()
	{
		PointU ptSqr = Sqr();
		return sqrt(ptSqr.x + ptSqr.y);
	}
	float InvLength()
	{
		return 1 / Length();
	}
	PointU Normalize()
	{
		PointU ptNorm;
		ptNorm.x = x * InvLength();
		ptNorm.y = y * InvLength();

		return ptNorm;
	}

	~PointU(){}
	USHORT x, y;
};

struct RectU
{
	RectU() :
		left(0),
		top(0),
		right(0),
		bottom(0)
	{}
	RectU(USHORT Left, USHORT Top, USHORT Right, USHORT Bottom) :
		left(Left),
		top(Top),
		right(Right),
		bottom(Bottom)
	{}
	RectU(RectU &&rect) :
		left(rect.left),
		top(rect.top),
		right(rect.right),
		bottom(rect.bottom)
	{
		ZeroMemory(&rect, sizeof(RectU));
	}

	RectU &operator=(RectU &&rect)
	{
		left = (rect.left);
		top = (rect.top);
		right = (rect.right);
		bottom = (rect.bottom);
		
		ZeroMemory(&rect, sizeof(RectU));
		return (*this);
	}

	~RectU()
	{}

	USHORT left, top, right, bottom;
};

struct WBClientData
{
	WBClientData()
		:
		tool(Tool::PaintBrush),
		clrIndex(0),
		mServ()
	{}
	Tool tool;								// Enums are sizeof(int) 4 bytes
	BYTE clrIndex;							// Palette color is 1 bytes
	MouseServer mServ;						// MouseServer is 16 - 32 bytes
};

