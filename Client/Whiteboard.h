#pragma once
#include "Timer.h"
#include "ColorDef.h"
#include "Palette.h"

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

class Whiteboard
{
public:
	Whiteboard(Palette& palette, HWND WinHandle, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex = 0);
	Whiteboard(Whiteboard&& wb);
	~Whiteboard();

	void Frame(RECT &rect, BYTE *pixelData);
private:
	void InitD3D();
	void BeginFrame();
	void ClearTempSurface();
	void Render(RECT &rect, BYTE *pixelData);
	void ComposeImage(USHORT Width, USHORT Height, BYTE *pixelData);
	void EndFrame();

	const Palette const &GetPalette(BYTE& count);

	HWND hWnd;

	USHORT width, height, pitch;
	float interval;
	Timer timer;
	D3DCOLOR bgColor;
	Palette& palette;

	IDirect3D9 *pDirect3D;
	IDirect3DDevice9 *pDevice;
	IDirect3DSurface9 *pBackBuffer;
	D3DCOLOR *tempSurface;
	D3DLOCKED_RECT lockRect;
};

