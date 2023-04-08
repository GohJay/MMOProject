#pragma once

struct SECTOR
{
	int x;
	int y;
};

struct SECTOR_AROUND
{
	int count;
	SECTOR around[9];
};
