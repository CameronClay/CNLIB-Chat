#pragma once

#include <vector>
#include <Windows.h>

#include "Mouse.h"

enum Tool
{
	PaintBrush, Line, Rect, FilledRect, ellipse, FilledEllipse
};

struct PointU
{
	USHORT x, y;
};

struct WBClientData
{
	Tool tool;								// Enums are sizeof(int) 4 bytes
	UINT clrIndex;							// Palette color is 4 bytes
	MouseServer mServ;						// MouseServer is 16 - 32 bytes
};

