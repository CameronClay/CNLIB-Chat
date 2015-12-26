//Copyright(c) <2015> <Cameron Clay>
#pragma once

//Minimum bytes required when compression is on BESTFIT to choose compression over raw data(compression byte cutoff)
#define COMPBYTECO 128

enum CompressionType
{
	BESTFIT,		//Chooses best choice for either compression you set or no compression
	SETCOMPRESSION, //Uses compression you set
	NOCOMPRESSION   //No compression
};
