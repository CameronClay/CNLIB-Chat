#pragma once
#include "CNLIB\TCPClientInterface.h"
#include "ColorDef.h"
#include "Palette.h"
#include "WhiteboardClientData.h"
#include <d3d9.h>

#pragma comment(lib, "d3d9.lib")

class Whiteboard
{
public:
	Whiteboard(TCPClientInterface& clint, Palette& palette, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex = 0);
	Whiteboard(Whiteboard&& wb);
	~Whiteboard();
	Whiteboard operator=(Whiteboard) = delete;

	void StartThread(TCPClientInterface* client);

	void Initialize(HWND WinHandle);
	void Frame(const RectU &rect, const BYTE *pixelData);
	void SendMouseData(TCPClientInterface* client);
	void BeginFrame();
	void EndFrame();

	//brushSize is relative
	void ChangeTool(Tool tool, float brushSize, BYTE palIndex);

	MouseServer& GetMServ();
	USHORT GetWidth() const;
	USHORT GetHeight() const;
	HANDLE GetTimer() const;
	BYTE GetDefaultColor() const;
	float GetBrushThickness() const;
private:
	void InitD3D();
	void Draw(const RectU &rect, const BYTE *pixelData);
	void ComposeImage(USHORT Width, USHORT Height, const BYTE *pixelData);

	const Palette& GetPalette(BYTE& count);


	TCPClientInterface& clint;
	MouseServer mouse;

	const BYTE defaultColor;
	BYTE palIndex;

	HWND hWnd;

	USHORT width, height, pitch;
	float brushThickness;

	const float interval;
	HANDLE timer, thread;
	DWORD threadID;

	Palette& palette;

	IDirect3D9 *pDirect3D;
	IDirect3DDevice9 *pDevice;
	IDirect3DSurface9 *pBackBuffer;
	D3DLOCKED_RECT lockRect;
};
