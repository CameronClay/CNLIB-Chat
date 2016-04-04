#pragma once
#include "CNLIB\TCPClientInterface.h"
#include "Palette.h"
#include "WhiteboardClientData.h"

class Whiteboard
{
public:
	Whiteboard(TCPClientInterface& clint, Palette& palette, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex = 0);
	Whiteboard(Whiteboard&& wb);
	~Whiteboard();
	Whiteboard operator=(Whiteboard) = delete;

	void Initialize(HWND WinHandle);

	void StartThread(TCPClientInterface* client);

	void Frame(const RectU &rect, const BYTE *pixelData);
	void Clear();
	void SendMouseData(TCPClientInterface* client);

	//brushSize is relative
	void ChangeTool(Tool tool, float brushSize, BYTE palIndex);

	MouseServer& GetMServ();
	HANDLE GetTimer() const;
	BYTE GetDefaultColor() const;
	float GetBrushThickness() const;

	class D3D
	{
	public:
		D3D(USHORT width, USHORT height);
		D3D(D3D&& d3d);
		~D3D();
		D3D operator=(D3D) = delete;

		void Initialize(HWND WinHandle);
		void Draw(const RectU &rect, const BYTE *pixelData, const Palette& palette);

		void Clear(D3DCOLOR clr);
		void Present();

		void BeginFrame();
		void EndFrame();

		IDirect3DDevice9* GetDevice(){ return pDevice; }
		USHORT GetWidth() const;
		USHORT GetHeight() const;
	private:
		IDirect3D9 *pDirect3D;
		IDirect3DDevice9 *pDevice;
		IDirect3DSurface9 *pBackBuffer;
		D3DLOCKED_RECT lockRect;
		HWND hWnd;

		USHORT width, height, pitch;
	};

	D3D& GetD3D();
private:
	D3D d3d;
	const Palette& GetPalette(BYTE& count);

	TCPClientInterface& clint;
	MouseServer mouse;

	const BYTE defaultColor;
	BYTE palIndex;

	float brushThickness;

	const float interval;
	HANDLE timer, thread;
	DWORD threadID;

	Palette& palette;
};
