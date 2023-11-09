#pragma once

//Minimum bytes required when compression is on BESTFIT to choose compression over raw data(compression byte cutoff)
#define COMPBYTECO 512

enum CompressionType
{
	BESTFIT,		//Chooses best choice for either compression you set or no compression
	SETCOMPRESSION, //Uses compression you set
	NOCOMPRESSION   //No compression
};
