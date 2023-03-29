#pragma once

#define dfDATABASE_LOG_WRITE_TERM			6000 * 10 * 10
#define dfNETWORK_NON_LOGIN_USER_TIMEOUT	1000 * 10

struct SERVER
{
	//-----------------------------------
	// CollectServer Info
	//-----------------------------------
	wchar_t collectIP[16];
	int collectPort;
	int collectWorkerCreateCnt;
	int collectWorkerRunningCnt;
	WORD collectSessionMax;

	//-----------------------------------
	// MonitorServer Info
	//-----------------------------------
	wchar_t monitorIP[16];
	int monitorPort;
	int monitorWorkerCreateCnt;
	int monitorWorkerRunningCnt;
	WORD monitorSessionMax;
	BYTE packetCode;
	BYTE packetKey;

};

struct SERVICE
{
	//-----------------------------------
	// MonitorServer LoginKey
	//-----------------------------------
	wchar_t loginKey[33];
};

struct SYSTEM
{
	//-----------------------------------
	// System Log
	//-----------------------------------
	int logLevel;
	wchar_t logPath[MAX_PATH / 2];
};

struct DATABASE
{
	//-----------------------------------
	// MySQL 접속 정보
	//-----------------------------------
	wchar_t ip[16];
	int port;
	wchar_t user[32];
	wchar_t passwd[32];
	wchar_t schema[32];
};
