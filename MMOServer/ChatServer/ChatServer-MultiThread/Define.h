#pragma once

#define dfSECTOR_MAX_X			50 + 1
#define dfSECTOR_MAX_Y			50 + 1
#define dfUNKNOWN_SECTOR		-1

struct SERVER
{
	//-----------------------------------
	// Listen IP / PORT
	//-----------------------------------
	wchar_t ip[16];
	int port;
	int workerCreateCnt;
	int workerRunningCnt;
	WORD sessionMax;
	WORD userMax;

	//-----------------------------------
	// Packet Encode Key
	//-----------------------------------
	BYTE packetCode;
	BYTE packetKey;

	//-----------------------------------
	// 미응답 유저 타임아웃 처리 (초 단위)
	//-----------------------------------
	int timeoutSec;
};

struct SYSTEM
{
	//-----------------------------------
	// System Log
	//-----------------------------------
	int logLevel;
	wchar_t logPath[MAX_PATH / 2];
};
