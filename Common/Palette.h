#pragma once

typedef DWORD GDICOLOR;
class Palette
{
public:
	Palette();
	Palette(Palette &&pal);
	D3DCOLOR *Get(BYTE &numColors);
	COLORREF GetBGRColor(BYTE index) const;
	D3DCOLOR GetRGBColor(BYTE index) const;

	~Palette();
private:
	D3DCOLOR *palette;
};
