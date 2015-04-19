#pragma once
#include "Tool.h"
#include "TCPServ.h"
#include <d2d1.h>
#include <wincodec.h>
#include <unordered_map>
#include "..\\Common\\ColorDef.h"
#include "..\\Common\\Mouse.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Windowscodecs.lib")

class Whiteboard
{
public:
	struct ClientData
	{
		Tool tool;								// Enums are sizeof(int) 4 bytes
		UINT clrIndex;							// Palette color is 4 bytes
		MouseServer mServ;						// MouseServer is 16 - 32 bytes
		std::vector<D2D1_POINT_2F> pointList;
	};

	Whiteboard(TCPServ &serv, UINT ScreenWidth, UINT ScreenHeight);
	~Whiteboard();

	void Draw(ClientData *pClientData);
	CRITICAL_SECTION & GetCritSection();
	std::unordered_map<Socket, Whiteboard::ClientData> & GetUMap();
private:
	UINT screenWidth, screenHeight;

	ID2D1Factory *pFactory;
	ID2D1RenderTarget *pRenderTarget;

	IWICImagingFactory *pWicFactory;
	IWICBitmap *pWicBitmap;

	CRITICAL_SECTION cs;
	std::unordered_map<Socket, Whiteboard::ClientData> clients;
};
