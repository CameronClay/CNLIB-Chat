#pragma once

#include <vector>
#include <deque>
#include <Windows.h>
#include "Vec2.h"

#include "Mouse.h"

enum class Tool
{
	PaintBrush, INVALID
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
	PointU operator*(const short val)
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

struct RectU
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

	inline static RectU Create(const Vec2& p0)
	{
		return{ (USHORT)p0.x, (USHORT)p0.y, (USHORT)p0.x, (USHORT)p0.y };
	}

	static RectU Create(const Vec2& p0, const Vec2& p1)
	{
		RectU rect;

		if(p1.x > p0.x)
		{
			rect.left = p0.x;
			rect.right = p1.x;
		}
		else
		{
			rect.left = p1.x;
			rect.right = p0.x;
		}

		if(p0.y > p1.y)
		{
			rect.top = p1.y;
			rect.bottom = p0.y;
		}
		else
		{
			rect.top = p0.y;
			rect.bottom = p1.y;
		}

		return rect;
	}

	void Modify(const Vec2& p0)
	{
		if(p0.x > right)
			right = p0.x;
		else if(p0.x < left)
			left = p0.x;
		if(p0.y > bottom)
			bottom = p0.y;
		else if(p0.y < top)
			top = p0.y;
	}

	void Modify(const Vec2& p0, const Vec2& p1)
	{
		if(p1.x > p0.x)
		{
			left = (p0.x > left) ? left : p0.x;
			right = (p1.x < right) ? right : p1.x;
		}
		else
		{
			left = (p1.x > left) ? left : p1.x;
			right = (p0.x < right) ? right : p0.x;
		}

		if(p0.y > p1.y)
		{
			top = (p1.y > top) ? top : p1.y;
			bottom = (p0.y < bottom) ? bottom : p0.y;
		}
		else
		{
			top = (p0.y > top) ? top : p0.y;
			bottom = (p1.y < bottom) ? bottom : p1.y;;
		}
	}
	USHORT left, top, right, bottom;
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
		mServ(50, 17),
		nVertices(0),
		tool(Tool::PaintBrush),
		clrIndex(0),
		thickness(1.0f),
		prevThickness(1.0f)
	{
		for(BYTE i = 0; i < 3; i++)
			vertices[i] = {};
	}

	WBClientData(USHORT FPS, BYTE defColor)
		:
		mServ((FPS < 60 ? 4500 / FPS : 75), (USHORT)((1000.0f / (float)FPS) + 0.5f)),
		nVertices(0),
		tool(Tool::PaintBrush),
		clrIndex(0),
		thickness(1.0f),
		prevThickness(1.0f)
	{
		do
		{
			clrIndex = rand() % 32;
		} while(clrIndex == defColor);

		for(BYTE i = 0; i < 3; i++)
			vertices[i] = {};
	}

	WBClientData(WBClientData&& clientData)
		:
		mServ(std::move(clientData.mServ)),
		nVertices(clientData.nVertices),
		tool(clientData.tool),
		clrIndex(clientData.clrIndex),
		thickness(clientData.thickness),
		prevThickness(clientData.prevThickness)
	{
		clientData.tool = Tool::INVALID;

		for(BYTE i = 0; i < 3; i++)
			vertices[i] = clientData.vertices[i];
	}

	~WBClientData(){}

	MouseServer mServ;
	Vec2 vertices[3];
	size_t nVertices;

	Tool tool;								// Enums are sizeof(int) 4 bytes
	BYTE clrIndex;							// Palette color is 1 bytes
	float thickness, prevThickness;

	static const BYTE UNCHANGEDCOLOR;
	static const float MINBRUSHSIZE;
	static const float MAXBRUSHSIZE;
};
