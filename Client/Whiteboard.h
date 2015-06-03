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
	Whiteboard(Palette& palette, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex = 0);
	Whiteboard(Whiteboard&& wb);
	~Whiteboard();

	void StartThread(TCPClientInterface* client);

	void Initialize(HWND WinHandle);
	void Frame(const RectU &rect, const BYTE *pixelData);
	void SendMouseData(TCPClientInterface* client);
	void BeginFrame();
	void EndFrame();
	void Render();

	MouseServer& GetMServ();
	USHORT GetWidth() const;
	USHORT GetHeight() const;
	HANDLE GetTimer() const;
private:
	void InitD3D();
	void Draw(const RECT &rect, const BYTE *pixelData);
	void ComposeImage(USHORT Width, USHORT Height, const BYTE *pixelData);

	const Palette& GetPalette(BYTE& count);

	MouseServer mouse;

	BYTE* surf;
	BYTE palIndex;

	HWND hWnd;

	USHORT width, height, pitch;

	const float interval;
	HANDLE timer, thread;
	DWORD threadID;

	Palette& palette;

	IDirect3D9 *pDirect3D;
	IDirect3DDevice9 *pDevice;
	IDirect3DSurface9 *pBackBuffer;
	D3DLOCKED_RECT lockRect;
};

