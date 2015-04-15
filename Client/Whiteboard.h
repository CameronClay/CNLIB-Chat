#pragma once
#include <d2d1.h>
#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Windowscodecs.lib")


class Whiteboard
{
public:
	Whiteboard();
	Whiteboard(HWND WinHandle, UINT Width, UINT Height);
	~Whiteboard();

	void Render(BYTE *pixelData, float Width, float Height);
private:
	HWND hWnd;
	ID2D1Factory *p2DFactory;
	ID2D1HwndRenderTarget *pHwndRenderTarget;
	ID2D1Bitmap *pBitmap;
	UINT width, height;
	D2D1_BITMAP_PROPERTIES bp;
	D2D1_RECT_U uRect;
};

