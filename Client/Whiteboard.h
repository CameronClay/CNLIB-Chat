#pragma once
#include "Timer.h"
#include "ColorDef.h"

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

//#include <d2d1.h>
//#pragma comment(lib, "d2d1.lib")
//#include <wincodec.h>
//#pragma comment(lib, "Windowscodecs.lib")

class Whiteboard
{
public:
	Whiteboard();
	Whiteboard(HWND WinHandle, USHORT Width, USHORT Height, USHORT FPS, UINT palIndex = Black);
	Whiteboard(Whiteboard&& wb);
	~Whiteboard();

	void Frame(RECT &rect, BYTE *pixelData);
private:
	void InitPalette();
	void InitD3D();
	void BeginFrame();
	void ClearTempSurface();
	void Render(RECT &rect, BYTE *pixelData);
	void ComposeImage(USHORT Width, USHORT Height, BYTE *pixelData);
	void EndFrame();

	D3DCOLOR* GetPalette(BYTE& count);

	HWND hWnd;

	USHORT width, height, pitch;
	float interval;
	Timer timer;
	D3DCOLOR bgColor;

	IDirect3D9 *pDirect3D;
	IDirect3DDevice9 *pDevice;
	IDirect3DSurface9 *pBackBuffer;
	D3DCOLOR *tempSurface;
	D3DLOCKED_RECT lockRect;
	D3DCOLOR palette[32];
	/*ID2D1Factory *p2DFactory;
	ID2D1HwndRenderTarget *pHwndRenderTarget;
	ID2D1Bitmap *pBitmap;
	D2D1_BITMAP_PROPERTIES bp;
	D2D1_RECT_U uRect;*/
};

