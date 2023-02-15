#pragma once

struct SERVER_INFO
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
	// System Log
	//-----------------------------------
	int logLevel;
	wchar_t logPath[MAX_PATH / 2];
};

struct SERVICE_INFO
{
	//-----------------------------------
	// ������ ���� Ÿ�Ӿƿ� ó�� (�� ����)
	//-----------------------------------
	int timeoutSec;
	wchar_t gameserver_ip[16];
	int gameserver_port;
	wchar_t chatserver_ip[16];
	int chatserver_port;
};

struct DATABASE_INFO
{
	//-----------------------------------
	// �����ͺ��̽� ���� ����
	//-----------------------------------
	wchar_t ip[16];
	int port;
	wchar_t user[32];
	wchar_t passwd[32];
	wchar_t schema[32];
};
