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
	USHORT x, y;
};

struct RectU
{
	USHORT left, top, right, bottom;
};

struct WBClientData
{
	Tool tool;								// Enums are sizeof(int) 4 bytes
	BYTE clrIndex;							// Palette color is 1 bytes
	MouseServer mServ;						// MouseServer is 16 - 32 bytes
};

