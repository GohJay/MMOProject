#pragma once

#define dfDATABASE_LOG_WRITE_TERM		6000 * 10 * 10

struct SERVER_INFO
{
	//-----------------------------------
	// CollectServer Info
	//-----------------------------------
	wchar_t collect_ip[16];
	int collect_port;
	int collect_workerCreateCnt;
	int collect_workerRunningCnt;
	WORD collect_sessionMax;

	//-----------------------------------
	// MonitorServer Info
	//-----------------------------------
	wchar_t monitor_ip[16];
	int monitor_port;
	int monitor_workerCreateCnt;
	int monitor_workerRunningCnt;
	WORD monitor_sessionMax;
	BYTE packetCode;
	BYTE packetKey;

	//-----------------------------------
	// MonitorServer LoginKey
	//-----------------------------------
	wchar_t loginKey[33];

	//-----------------------------------
	// System Log
	//-----------------------------------
	int logLevel;
	wchar_t logPath[MAX_PATH / 2];
};

struct DATABASE_INFO
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
