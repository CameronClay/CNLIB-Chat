#pragma once
#include "Tool.h"
#include "TCPServ.h"
#include <d2d1.h>
#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Windowscodecs.lib")
class Whiteboard
{
public:
	Whiteboard(TCPServ &serv);
	~Whiteboard();

	void Draw(Tool *pTool)
	{
		pTool->Draw();
	}
	void GetBitmap();
private:
	ID2D1Factory *pFactory;
	ID2D1RenderTarget *pRenderTarget;
	IWICImagingFactory *pWicFactory;
	IWICBitmap *pWicBitmap;
	CRITICAL_SECTION cs;
};

/*
Pseudo code
Client:

*/