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
	void BeginFrame();
	void EndFrame();
	void Render();
	bool MouseInterval() const;
private:
	void InitD3D();
	void Draw(const RECT &rect, const BYTE *pixelData);
	void ComposeImage(USHORT Width, USHORT Height, const BYTE *pixelData);

	const Palette& GetPalette(BYTE& count);

	BYTE* surf;
	HWND hWnd;

	USHORT width, height, pitch;
	const float interval;
	Timer timer;
	Timer mouseTimer;
	Palette& palette;

	IDirect3D9 *pDirect3D;
	IDirect3DDevice9 *pDevice;
	IDirect3DSurface9 *pBackBuffer;
	D3DLOCKED_RECT lockRect;
};

