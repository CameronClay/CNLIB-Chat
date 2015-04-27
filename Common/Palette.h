#pragma once
#include <d3d9.h>

class Palette
{
public:
	Palette();
	D3DCOLOR *Get(BYTE &numColors);
	D3DCOLOR GetColor(BYTE index);
	~Palette();
private:
	D3DCOLOR *palette;
};