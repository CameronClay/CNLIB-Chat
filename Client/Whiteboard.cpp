#include "Whiteboard.h"
#include <assert.h>

Whiteboard::Whiteboard()
{
}

Whiteboard::Whiteboard(HWND WinHandle, UINT Width, UINT Height) :
hWnd(WinHandle),
width(Width),
height(Height),
bp(D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED))),
uRect(D2D1::RectU(0, 0, Width, Height))
{
	HRESULT hr = 0;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &p2DFactory);
	assert(SUCCEEDED(hr));

	hr = p2DFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(), 
		D2D1::HwndRenderTargetProperties(hWnd), 
		&pHwndRenderTarget);

	D2D1_SIZE_U bmpSize = D2D1::SizeU(Width, Height);
	hr = pHwndRenderTarget->CreateBitmap(bmpSize, bp, &pBitmap);
}
void Whiteboard::Render(BYTE* pixelData, float Width, float Height)
{
	HRESULT hr = 0;
	UINT pitch = Width * 4;
	
	hr = pBitmap->CopyFromMemory(&uRect, pixelData, pitch);
	assert(SUCCEEDED(hr));

	pHwndRenderTarget->BeginDraw();

	D2D1_RECT_F rect = D2D1::RectF(0.0f, 0.0f, Width, Height);
	pHwndRenderTarget->DrawBitmap(pBitmap, rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
		rect);

	hr = pHwndRenderTarget->EndDraw();
	assert(SUCCEEDED(hr));
}

Whiteboard::~Whiteboard()
{
}
