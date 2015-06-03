#pragma once

#include <vector>
#include <deque>
#include <Windows.h>

#include "Mouse.h"

enum class Tool
{
	PaintBrush, Line, Rect, FilledRect, ellipse, FilledEllipse, INVALID
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
	PointU operator*(const float val)
	{
		PointU temp;
		temp.x = (USHORT)((float)x * val);
		temp.y = (USHORT)((float)x * val);

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
		return sqrtf((float)(ptSqr.x + ptSqr.y));
	}
	float InvLength()
	{
		return 1 / Length();
	}
	PointU Normalize()
	{
		PointU ptNorm;
		ptNorm.x = (USHORT)((float)x * InvLength());
		ptNorm.y = (USHORT)((float)y * InvLength());

		return ptNorm;
	}

	~PointU(){}
	USHORT x, y;
};

struct RectU :RECT
{
	RectU()
	{
		left = 0;
		top = 0;
		right = 0;
		bottom = 0;
	}
	RectU(USHORT Left, USHORT Top, USHORT Right, USHORT Bottom)
	{
		left = Left;
		right = Right;
		top = Top;
		bottom = Bottom;
	}
	RectU(RectU &&rect)
	{
		left = rect.left;
		top = rect.top;
		right = rect.right;
		bottom = rect.bottom;

		ZeroMemory(&rect, sizeof(RectU));
	}

	RectU &operator=(RectU &&rect)
	{
		left = rect.left;
		top = rect.top;
		right = rect.right;
		bottom = rect.bottom;

		ZeroMemory(&rect, sizeof(RectU));
		return (*this);
	}

	~RectU()
	{}
};


struct WBParams
{
	WBParams()
		:
		width(800),
		height(600),
		fps(20),
		clrIndex(0)
	{}
	WBParams(USHORT width, USHORT height, USHORT fps, BYTE clrIndex)
		:
		width(width),
		height(height),
		fps(fps),
		clrIndex(clrIndex)
	{}
	WBParams(WBParams&& params)
		:
		width(params.width),
		height(params.height),
		fps(params.fps),
		clrIndex(params.clrIndex)
	{}
	USHORT width, height, fps;
	BYTE clrIndex;
};

struct WBClientData
{
	WBClientData()
		:
		rect(),
		tool(Tool::PaintBrush),
		clrIndex(0),
		mServ(100)
	{}

	WBClientData(USHORT FPS)
		:
		rect(),
		tool(Tool::PaintBrush),
		clrIndex(0),
		mServ((FPS >= 60 ? 6000 / FPS : 1000))
	{}

	WBClientData(WBClientData&& clientData)
		:
		pointList(std::move(clientData.pointList)),
		rect(clientData.rect),
		tool(clientData.tool),
		clrIndex(clientData.clrIndex),
		mServ(std::move(clientData.mServ))
	{
		clientData.tool = Tool::INVALID;
	}

	~WBClientData(){}

	std::deque<PointU> pointList;
	RectU rect;
	Tool tool;								// Enums are sizeof(int) 4 bytes
	BYTE clrIndex;							// Palette color is 1 bytes
	MouseServer mServ;						
};
