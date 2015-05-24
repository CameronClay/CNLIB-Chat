#pragma once
#include "CNLIB\TCPClientInterface.h"
#include "Timer.h"
#include "ColorDef.h"
#include "Palette.h"
#include "WhiteboardClientData.h"
#include <d3d9.h>

#pragma comment(lib, "d3d9.lib")

class Whiteboard
{
public:
	Whiteboard(Palette& palette, HWND WinHandle, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex = 0);
	Whiteboard(Whiteboard&& wb);
	~Whiteboard();

	void Frame(const RectU &rect, const BYTE *pixelData);
	void SendMouseData(MouseServer& mServ, TCPClientInterface* client);
	bool Interval() const;
private:
	void InitD3D();
	void BeginFrame();
	void ClearTempSurface();
	void Render(const RECT &rect, const BYTE *pixelData);
	void ComposeImage(USHORT Width, USHORT Height, const BYTE *pixelData);
	void EndFrame();

	const Palette& GetPalette(BYTE& count);

	HWND hWnd;

	USHORT width, height, pitch;
	const float interval;
	Timer timer;
	D3DCOLOR bgColor;
	Palette& palette;

	IDirect3D9 *pDirect3D;
	IDirect3DDevice9 *pDevice;
	IDirect3DSurface9 *pBackBuffer;
	D3DCOLOR *tempSurface;
	D3DLOCKED_RECT lockRect;
};

