#pragma once

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

struct CLIENT
{
	//-----------------------------------
	// MonitorServer Connect IP / PORT
	//-----------------------------------
	wchar_t ip[16];
	int port;
	bool reconnect;
};

struct SYSTEM
{
	//-----------------------------------
	// System Log
	//-----------------------------------
	int logLevel;
	wchar_t logPath[MAX_PATH / 2];
};
