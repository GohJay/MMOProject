#include "stdafx.h"
#include "../../Common/CrashDump.h"
#include "../../Common/Logger.h"
#include "MainServer.h"
#include "AuthServer.h"
#include "GameServer.h"
#include "MonitorClient.h"
#include "ServerConfig.h"
#pragma comment(lib, "Winmm.lib")

MainServer g_MainServer;
AuthServer g_AuthServer(&g_MainServer);
GameServer g_GameServer(&g_MainServer);
MonitorClient g_MonitorClient(&g_MainServer, &g_AuthServer, &g_GameServer);

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

	if (!g_MainServer.Start(ServerConfig::GetGameServerIP()
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

	g_MainServer.Stop();
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
------------------------------------\n\
Total Accept: %lld\n\
Accept TPS: %d\n\
Recv TPS: %d\n\
Send TPS: %d\n\
------------------------------------\n\
\n\n\n\n\n\n\n\n\n\n\n\n\n"
		, stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday, stTime.tm_hour, stTime.tm_min, stTime.tm_sec
		, g_MainServer.GetCapacityPacketPool()
		, g_MainServer.GetUsePacketPool()
		, g_MainServer.GetSessionCount()
		, g_AuthServer.GetPlayerCount()
		, g_GameServer.GetPlayerCount()
		, g_AuthServer.GetFPS()
		, g_GameServer.GetFPS()
		, g_MainServer.GetTotalAcceptCount()
		, g_MainServer.GetAcceptTPS()
		, g_MainServer.GetRecvTPS()
		, g_MainServer.GetSendTPS());
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
