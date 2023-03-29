#include "stdafx.h"
#include "../../Common/CrashDump.h"
#include "../../Common/Logger.h"
#include "LoginServer.h"
#include "MonitorClient.h"
#include "ServerConfig.h"
#pragma comment(lib, "Winmm.lib")

LoginServer g_LoginServer;
MonitorClient g_MonitorClient(&g_LoginServer);
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

	if (!g_LoginServer.Start(ServerConfig::GetLoginServerIP()
		, ServerConfig::GetLoginServerPort()
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

	g_LoginServer.Stop();
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
Total Accept: %d\n\
Accept TPS: %d\n\
Recv TPS: %d\n\
Send TPS: %d\n\
------------------------------------\n\
Auth TPS: %d\n\
------------------------------------\n\
\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		, stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday, stTime.tm_hour, stTime.tm_min, stTime.tm_sec
		, g_LoginServer.GetCapacityPacketPool()
		, g_LoginServer.GetUsePacketPool()
		, g_LoginServer.GetSessionCount()
		, g_LoginServer.GetTotalAcceptCount()
		, g_LoginServer.GetAcceptTPS()
		, g_LoginServer.GetRecvTPS()
		, g_LoginServer.GetSendTPS()
		, g_LoginServer.GetAuthTPS());
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
			wprintf_s(L"Control Mode: Press M - Service Mode\n");
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

		if ((controlKey == L'm' || controlKey == L'M') && g_ControlMode)
		{
			g_LoginServer.SwitchServiceMode();
			wprintf_s(L"Control - Switch To Service Mode\n");
		}
	}
}
