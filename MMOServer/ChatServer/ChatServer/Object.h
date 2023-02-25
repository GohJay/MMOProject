#pragma once
#include "Define.h"
#include <Windows.h>

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

struct USER
{
	DWORD64 sessionID;
	INT64 accountNo;
	WCHAR id[20];
	WCHAR nickname[20];
	bool login;
};

struct PLAYER
{
	DWORD64 sessionID;
	INT64 accountNo;
	SECTOR sector;
};
