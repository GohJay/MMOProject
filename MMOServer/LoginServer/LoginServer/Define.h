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

struct SERVICE
{
	//-----------------------------------
	// MMO 서버 정보
	//-----------------------------------
	wchar_t gameserver_ip[16];
	int gameserver_port;
	wchar_t chatserver_ip[16];
	int chatserver_port;
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

	//-----------------------------------
	// Redis 접속 정보
	//-----------------------------------
	wchar_t redis_ip[16];
	int redis_port;
	int redis_timeout_sec;
};
