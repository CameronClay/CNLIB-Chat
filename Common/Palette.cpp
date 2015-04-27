#include "Palette.h"
#include "ColorDef.h"
#include "HeapAlloc.h"

Palette::Palette()
{
	palette = alloc<D3DCOLOR>(32);
	BYTE i = 0;
	palette[i++] = Black;
	palette[i++] = DarkGray;
	palette[i++] = LightGray;
	palette[i++] = White;
	palette[i++] = DarkRed;
	palette[i++] = MediumRed;
	palette[i++] = Red;
	palette[i++] = LightRed;
	palette[i++] = DarkOrange;
	palette[i++] = MediumOrange;
	palette[i++] = Orange;
	palette[i++] = LightOrange;
	palette[i++] = DarkYellow;
	palette[i++] = MediumYellow;
	palette[i++] = Yellow;
	palette[i++] = LightYellow;
	palette[i++] = DarkGreen;
	palette[i++] = MediumGreen;
	palette[i++] = Green;
	palette[i++] = LightGreen;
	palette[i++] = DarkCyan;
	palette[i++] = MediumCyan;
	palette[i++] = Cyan;
	palette[i++] = LightCyan;
	palette[i++] = DarkBlue;
	palette[i++] = MediumBlue;
	palette[i++] = Blue;
	palette[i++] = LightBlue;
	palette[i++] = DarkPurple;
	palette[i++] = MediumPurple;
	palette[i++] = Purple;
	palette[i] = LightPurple;
}

D3DCOLOR *Palette::Get(BYTE &numColors)
{
	numColors = 32;
	return palette;
}

D3DCOLOR Palette::GetColor(BYTE index)
{
	return palette[index];
}

Palette::~Palette()
{
	dealloc(palette);
}