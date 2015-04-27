#pragma once
#include <d3d9.h>


class Palette
{
public:
	Palette();
	D3DCOLOR *Get(BYTE &numColors);
	GDICOLOR GetBGRColor(BYTE index);
	D3DCOLOR GetRGBColor(BYTE index);

	~Palette();
private:
	D3DCOLOR *palette;
};

typedef int GDICOLOR;