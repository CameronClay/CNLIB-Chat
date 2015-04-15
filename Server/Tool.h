#pragma once
#include <vector>
#include <d2d1.h>

class Tool
{
public:
	Tool();
	virtual ~Tool();

	virtual void Draw() abstract;

protected:
	std::vector<D2D1_POINT_2F> pointList;
	D2D1_COLOR_F color;

};

