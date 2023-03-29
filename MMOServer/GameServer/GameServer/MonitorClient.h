#pragma once
#include "MainServer.h"
#include "AuthServer.h"
#include "GameServer.h"
#include "../../Common/ProcessUsage.h"
#include "../../Common/ProcesserUsage.h"
#include "../../Lib/Network/include/LanClient.h"
#include <thread>

enum MONITOR_STATUS
{
	CONNECT = 0,
	DISCONNECT,
	LOGIN,
	MONITORING,
	STOP,
};

class MonitorClient : Jay::LanClient
{
public:
	MonitorClient(MainServer* main, AuthServer* auth, GameServer* game);
	~MonitorClient();
protected:
	void OnEnterJoinServer() override;
	void OnLeaveServer() override;
	void OnRecv(Jay::NetPacket* packet) override;
	void OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam) override;
public:
	void MonitoringThread();
	void ConnectProc();
	void DisconnectProc();
	void LoginProc();
	void MonitoringProc();
	void Update(BYTE dataType, int dataValue, int timeStamp);
private:
	MainServer* _main;
	AuthServer* _auth;
	GameServer* _game;
	Jay::ProcessUsage _processUsage;
	Jay::ProcesserUsage _processerUsage;
	std::thread _monitoringThread;
	MONITOR_STATUS _status;
	HANDLE _hExitEvent;
};
