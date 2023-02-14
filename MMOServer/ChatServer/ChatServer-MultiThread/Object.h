#pragma once
#include "Define.h"
#include "../../Common/Lock.h"
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

struct CHARACTER
{
	DWORD64 sessionID;
	INT64 accountNo;
	WCHAR id[20];
	WCHAR nickname[20];
	char sessionKey[64];
	bool login;
	SECTOR sector;
	Jay::SRWLock lock;
};
