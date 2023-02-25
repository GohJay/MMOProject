#include "stdafx.h"
#include "../../Common/CrashDump.h"
#include "../../Common/Logger.h"
#include "CollectServer.h"
#include "MonitorServer.h"
#include "ServerConfig.h"
#pragma comment(lib, "Winmm.lib")

MonitorServer g_MonitorServer;
CollectServer g_CollectServer(&g_MonitorServer);
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

	if (!g_MonitorServer.Start(ServerConfig::GetMonitorServerIP()
		, ServerConfig::GetMonitorServerPort()
		, ServerConfig::GetMonitorIOCPWorkerCreate()
		, ServerConfig::GetMonitorIOCPWorkerRunning()
		, ServerConfig::GetMonitorSessionMax()
		, ServerConfig::GetPacketCode()
		, ServerConfig::GetPacketKey()))
		return;

	if (!g_CollectServer.Start(ServerConfig::GetCollectServerIP()
		, ServerConfig::GetCollectServerPort()
		, ServerConfig::GetCollectIOCPWorkerCreate()
		, ServerConfig::GetCollectIOCPWorkerRunning()
		, ServerConfig::GetCollectSessionMax()))
		return;

	while (!g_StopSignal)
	{
		Monitor();
		Control();
		Sleep(1000);
	}

	g_CollectServer.Stop();
	g_MonitorServer.Stop();
}

bool Init()
{
	if (!ServerConfig::LoadFile(L"Config.cnf"))
		return false;

	Jay::Logger::SetLogLevel(ServerConfig::GetLogLevel());
	Jay::Logger::SetLogPath(ServerConfig::GetLogPath());
	return true;
}

void Monitor()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	wprintf_s(L"\
[%d/%02d/%02d %02d:%02d:%02d]\n\
------------------------------------\n\
Session Count: %d\n\
Packet Pool Use: %d\n\
------------------------------------\n\
Total Accept: %d\n\
Accept TPS: %d\n\
Recv TPS: %d\n\
Send TPS: %d\n\
------------------------------------\n\
\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		, stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday, stTime.tm_hour, stTime.tm_min, stTime.tm_sec
		, g_CollectServer.GetSessionCount()
		, g_CollectServer.GetUsePacketCount()
		, g_CollectServer.GetTotalAcceptCount()
		, g_CollectServer.GetAcceptTPS()
		, g_CollectServer.GetRecvTPS()
		, g_CollectServer.GetSendTPS());
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
