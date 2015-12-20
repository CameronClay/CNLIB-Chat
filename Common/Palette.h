#pragma once
#include <d3d9.h>

typedef DWORD GDICOLOR;
class Palette
{
public:
	Palette();
	Palette(Palette &&pal);
	D3DCOLOR *Get(BYTE &numColors);
	COLORREF GetBGRColor(BYTE index);
	D3DCOLOR GetRGBColor(BYTE index);

	~Palette();
private:
	D3DCOLOR *palette;
};
