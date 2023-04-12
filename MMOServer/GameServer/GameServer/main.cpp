#include "stdafx.h"
#include "../../Common/CrashDump.h"
#include "../../Common/Logger.h"
#include "GameServer.h"
#include "AuthContent.h"
#include "GameContent.h"
#include "MonitorClient.h"
#include "ServerConfig.h"
#pragma comment(lib, "Winmm.lib")

GameServer g_GameServer;
AuthContent g_AuthContent(&g_GameServer);
GameContent g_GameContent(&g_GameServer);
MonitorClient g_MonitorClient(&g_GameServer, &g_AuthContent, &g_GameContent);

time_t g_StartTime;
bool g_StopSignal = false;
bool g_ControlMode = false;

void Run();
bool Init();
void Monitor();
void Control();

int main()
{
	timeBeginPeriod(1);

	Run();

	wprintf_s(L"Press any key to continue . . . ");
	_getwch();

	timeEndPeriod(1);
	return 0;
}

void Run()
{
	if (!Init())
		return;

	if (!g_GameServer.Start(ServerConfig::GetGameServerIP()
		, ServerConfig::GetGameServerPort()
		, ServerConfig::GetIOCPWorkerCreate()
		, ServerConfig::GetIOCPWorkerRunning()
		, ServerConfig::GetSessionMax()
		, ServerConfig::GetPacketCode()
		, ServerConfig::GetPacketKey()
		, ServerConfig::GetSessionTimeoutSec()))
		return;

	while (!g_StopSignal)
	{
		Monitor();
		Control();
		Sleep(1000);
	}

	g_GameServer.Stop();
}

bool Init()
{
	if (!ServerConfig::LoadFile(L"Config.cnf"))
		return false;

	Jay::Logger::SetLogLevel(ServerConfig::GetLogLevel());
	Jay::Logger::SetLogPath(ServerConfig::GetLogPath());

	g_StartTime = time(NULL);
	return true;
}

void Monitor()
{
	tm stTime;
	localtime_s(&stTime, &g_StartTime);

	wprintf_s(L"\
StartTime: %d/%02d/%02d %02d:%02d:%02d\n\
------------------------------------\n\
Packet Pool Capacity: %d\n\
Packet Pool Use: %d\n\
Session Count: %d\n\
------------------------------------\n\
Auth Player Count: %d\n\
Game Player Count: %d\n\
Auth FPS: %d\n\
Game FPS: %d\n\
DB Write TPS: %d\n\
DB Job Queue Count: %d\n\
------------------------------------\n\
Total Accept: %lld\n\
Accept TPS: %d\n\
Recv TPS: %d\n\
Send TPS: %d\n\
------------------------------------\n\
\n\n\n\n\n\n\n\n\n\n\n"
		, stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday, stTime.tm_hour, stTime.tm_min, stTime.tm_sec
		, g_GameServer.GetCapacityPacketPool()
		, g_GameServer.GetUsePacketPool()
		, g_GameServer.GetSessionCount()
		, g_AuthContent.GetPlayerCount()
		, g_GameContent.GetPlayerCount()
		, g_AuthContent.GetFPS()
		, g_GameContent.GetFPS()
		, g_GameContent.GetDBWriteTPS()
		, g_GameContent.GetDBJobQueueCount()
		, g_GameServer.GetTotalAcceptCount()
		, g_GameServer.GetAcceptTPS()
		, g_GameServer.GetRecvTPS()
		, g_GameServer.GetSendTPS());
}

void Control()
{
	wchar_t controlKey;
	if (_kbhit())
	{
		controlKey = _getwch();

		// 키보드 제어 허용
		if (controlKey == L'u' || controlKey == L'U')
		{
			g_ControlMode = true;
			wprintf_s(L"Control Mode: Press Q - Quit\n");
			wprintf_s(L"Control Mode: Press L - Key Lock\n");
		}

		// 키보드 제어 잠금
		if ((controlKey == L'l' || controlKey == L'L') && g_ControlMode)
		{
			g_ControlMode = false;
			wprintf_s(L"Control Lock! Press U - Key UnLock\n");
		}

		// 키보드 제어 풀림 상태에서의 특정 기능
		if ((controlKey == L'q' || controlKey == L'Q') && g_ControlMode)
		{
			g_StopSignal = true;
		}
	}
}
