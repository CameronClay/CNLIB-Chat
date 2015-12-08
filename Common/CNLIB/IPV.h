#pragma once

enum IPv : unsigned short
{
	ipvnone,
	ipv4 = 0xFF00,
	ipv6 = 0x00FF,
	ipboth = 0xFFFF
};