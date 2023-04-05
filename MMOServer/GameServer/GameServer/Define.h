#pragma once

//-----------------------------------------------------------------
// �� Ÿ�� ����
//-----------------------------------------------------------------
#define dfMAP_TILE_MAX_X					400
#define dfMAP_TILE_MAX_Y					200

//-----------------------------------------------------------------
// �� ���� ����
//-----------------------------------------------------------------
#define dfSECTOR_SIZE_X						10
#define dfSECTOR_SIZE_Y						5

#define dfSECTOR_MAX_X						(dfMAP_TILE_MAX_X / dfSECTOR_SIZE_X) + 1
#define dfSECTOR_MAX_Y						(dfMAP_TILE_MAX_Y / dfSECTOR_SIZE_Y) + 1

//-----------------------------------------------------------------
// �÷��̾� �⺻ ����
//-----------------------------------------------------------------
#define dfPLAYER_HP_DEFAULT					5000
#define dfMINUS_CRISTAL						1000

//-----------------------------------------------------------------
// �÷��̾� ������ Ÿ�� ����
//-----------------------------------------------------------------
#define dfPLAYER_TILE_X_RESPAWN_CENTER		95
#define dfPLAYER_TILE_Y_RESPAWN_CENTER		108

//-----------------------------------------------------------------
// ���� �⺻ ����
//-----------------------------------------------------------------
#define dfMONSTER_RESPAWN_TIME				15000

#define dfMONSTER_RESPAWN_COUNT_ZONE_1		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_2		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_3		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_4		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_5		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_6		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_7		20

//-----------------------------------------------------------------
// ���� ������ Ÿ�� ����
//-----------------------------------------------------------------
#define dfMONSTER_TILE_X_RESPAWN_CENTER_1	26
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_1	169
#define dfMONSTER_TILE_X_RESPAWN_CENTER_2	95
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_2	169
#define dfMONSTER_TILE_X_RESPAWN_CENTER_3	163
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_3	152
#define dfMONSTER_TILE_X_RESPAWN_CENTER_4	171
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_4	33
#define dfMONSTER_TILE_X_RESPAWN_CENTER_5	34
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_5	61
#define dfMONSTER_TILE_X_RESPAWN_CENTER_6	219
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_6	179
#define dfMONSTER_TILE_X_RESPAWN_CENTER_7	317
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_7	114

//-----------------------------------------------------------------
// ũ����Ż �⺻ ����
//-----------------------------------------------------------------
#define dfCRISTAL_TIME_LIMIT				30000

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
	// ������ ���� Ÿ�Ӿƿ� ó�� (�� ����)
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

struct DATABASE
{
	//-----------------------------------
	// MySQL ���� ����
	//-----------------------------------
	wchar_t ip[16];
	int port;
	wchar_t user[32];
	wchar_t passwd[32];
	wchar_t schema[32];

	//-----------------------------------
	// Redis ���� ����
	//-----------------------------------
	wchar_t redis_ip[16];
	int redis_port;
};