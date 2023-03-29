#include "stdafx.h"
#include "MonitorClient.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "../../Common/Logger.h"
#include "../../Common/CrashDump.h"
#include "../../Common/CommonProtocol.h"
#include "../../Lib/Network/include/Error.h"
#pragma comment(lib, "../../Lib/Network/lib64/Network.lib")

using namespace Jay;

MonitorClient::MonitorClient(MainServer* main, AuthServer* auth, GameServer* game) : _main(main), _auth(auth), _game(game), _status(CONNECT)
{
	_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_monitoringThread = std::thread(&MonitorClient::MonitoringThread, this);
}
MonitorClient::~MonitorClient()
{
	SetEvent(_hExitEvent);
	_monitoringThread.join();

	LanClient::Disconnect();
	CloseHandle(_hExitEvent);
}
void MonitorClient::OnEnterJoinServer()
{
}
void MonitorClient::OnLeaveServer()
{
}
void MonitorClient::OnRecv(Jay::NetPacket* packet)
{
}
void MonitorClient::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------
	// Network IO Error 로깅
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Monitor"
		, LOG_LEVEL_ERROR
		, L"func: %s, line: %d, error: %d, wParam: %llu, lParam: %llu"
		, funcname, linenum, errcode, wParam, lParam);

	//--------------------------------------------------------------------
	// Fatal Error 일 경우 크래시와 함께 메모리 덤프를 남긴다.
	//--------------------------------------------------------------------
	if (errcode >= NET_FATAL_INVALID_SIZE)
		CrashDump::Crash();
}
void MonitorClient::MonitoringThread()
{
	DWORD ret;
	while (1)
	{
		ret = WaitForSingleObject(_hExitEvent, 1000);
		if (ret != WAIT_TIMEOUT)
			break;

		switch (_status)
		{
		case CONNECT:
			ConnectProc();
			break;
		case DISCONNECT:
			DisconnectProc();
			break;
		case LOGIN:
			LoginProc();
			break;
		case MONITORING:
			MonitoringProc();
			break;
		case STOP:
			return;
		default:
			break;
		}
	}
}
void MonitorClient::ConnectProc()
{
	if (LanClient::Connect(ServerConfig::GetMonitorServerIP(), ServerConfig::GetMonitorServerPort()))
		_status = LOGIN;
	else
		_status = DISCONNECT;
}
void MonitorClient::DisconnectProc()
{
	if (ServerConfig::IsMonitorReconnect())
		_status = CONNECT;
	else
		_status = STOP;
}
void MonitorClient::LoginProc()
{
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMonitorLogin(packet, dfMONITOR_SERVER_NO_GAME);
	LanClient::SendPacket(packet);

	NetPacket::Free(packet);
	_status = MONITORING; 
}
void MonitorClient::MonitoringProc()
{
	// 현재 시간 구하기
	time_t timer = time(NULL);

	// 리소스 값 갱신
	_processUsage.Update();
	_processerUsage.Update();

	// 프로세스 사용률 갱신
	Update(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, 1, timer);
	Update(dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, _processUsage.GetUseCPUTotalTime(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, _processUsage.GetUseMemoryMBytes(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_SESSION, _main->GetSessionCount(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, _auth->GetPlayerCount(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, _game->GetPlayerCount(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, _main->GetAcceptTPS(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, _main->GetRecvTPS(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, _main->GetSendTPS(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, _auth->GetFPS(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, _game->GetFPS(), timer);
	Update(dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, _main->GetUsePacketPool(), timer);

	// 프로세서 사용률 갱신
	Update(dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, _processerUsage.GetUseCPUTotalTime(), timer);
	Update(dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, _processerUsage.GetUseNonpagedPoolMBytes(), timer);
	Update(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, _processerUsage.GetNetworkRecvKBytes(), timer);
	Update(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, _processerUsage.GetNetworkSendKBytes(), timer);
	Update(dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, _processerUsage.GetFreeMemoryMBytes(), timer);
}
void MonitorClient::Update(BYTE dataType, int dataValue, int timeStamp)
{
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMonitorDataUpdate(packet, dataType, dataValue, timeStamp);
	if (!LanClient::SendPacket(packet))
		_status = DISCONNECT;

	NetPacket::Free(packet);
}
