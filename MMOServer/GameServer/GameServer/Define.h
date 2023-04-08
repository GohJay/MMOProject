#pragma once

//-----------------------------------------------------------------
// 맵 타일 정보
//-----------------------------------------------------------------
#define dfMAP_TILE_MAX_X					400
#define dfMAP_TILE_MAX_Y					200

//-----------------------------------------------------------------
// 맵 섹터 정보
//-----------------------------------------------------------------
#define dfSECTOR_SIZE_X						20
#define dfSECTOR_SIZE_Y						10

#define dfSECTOR_MAX_X						(dfMAP_TILE_MAX_X / dfSECTOR_SIZE_X) + 1
#define dfSECTOR_MAX_Y						(dfMAP_TILE_MAX_Y / dfSECTOR_SIZE_Y) + 1

//-----------------------------------------------------------------
// 플레이어 기본 정보
//-----------------------------------------------------------------
#define dfPLAYER_HP_DEFAULT					5000

#define dfPLAYER_HP_RECOVER_COOLING_TIME	1000

#define dfPLAYER_DIE_MINUS_CRISTAL			1000

//-----------------------------------------------------------------
// 플레이어 리스폰 타일 정보
//-----------------------------------------------------------------
#define dfPLAYER_TILE_X_RESPAWN_CENTER		95
#define dfPLAYER_TILE_Y_RESPAWN_CENTER		108

//-----------------------------------------------------------------
// 몬스터 기본 정보
//-----------------------------------------------------------------
#define dfMONSTER_SIGHT_RANGE_X				8
#define dfMONSTER_SIGHT_RANGE_Y				4

#define dfMONSTER_ATTACK_RANGE_X			1
#define dfMONSTER_ATTACK_RANGE_Y			1

#define dfMONSTER_MOVE_RANDOM_CHANCE		2

#define dfMONSTER_SPEED_X					0.5f
#define dfMONSTER_SPEED_Y					0.5f

#define dfMONSTER_MOVE_COOLING_TIME			1000
#define dfMONSTER_ATTACK_COOLING_TIME		2000
#define dfMONSTER_RESPAWN_TIME				15000

#define dfMONSTER_RESPAWN_COUNT_ZONE_1		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_2		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_3		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_4		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_5		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_6		10
#define dfMONSTER_RESPAWN_COUNT_ZONE_7		20

//-----------------------------------------------------------------
// 몬스터 리스폰 타일 정보
//-----------------------------------------------------------------
#define dfMONSTER_TILE_X_RESPAWN_CENTER_1	26
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_1	169
#define dfMONSTER_TILE_RESPAWN_RANGE_1		24

#define dfMONSTER_TILE_X_RESPAWN_CENTER_2	95
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_2	169
#define dfMONSTER_TILE_RESPAWN_RANGE_2		17

#define dfMONSTER_TILE_X_RESPAWN_CENTER_3	163
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_3	152
#define dfMONSTER_TILE_RESPAWN_RANGE_3		25

#define dfMONSTER_TILE_X_RESPAWN_CENTER_4	171
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_4	33
#define dfMONSTER_TILE_RESPAWN_RANGE_4		17

#define dfMONSTER_TILE_X_RESPAWN_CENTER_5	34
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_5	61
#define dfMONSTER_TILE_RESPAWN_RANGE_5		17

#define dfMONSTER_TILE_X_RESPAWN_CENTER_6	219
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_6	179
#define dfMONSTER_TILE_RESPAWN_RANGE_6		14

#define dfMONSTER_TILE_X_RESPAWN_CENTER_7	317
#define dfMONSTER_TILE_Y_RESPAWN_CENTER_7	114
#define dfMONSTER_TILE_RESPAWN_RANGE_7		79

//-----------------------------------------------------------------
// 크리스탈 기본 정보
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
};
