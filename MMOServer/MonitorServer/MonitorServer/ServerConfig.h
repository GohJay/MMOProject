#pragma once
#include "Define.h"

class ServerConfig
{
public:
	static bool LoadFile(const wchar_t* filename);
	static const wchar_t* GetCollectServerIP() { return _serverInfo.collect_ip; }
	static int GetCollectServerPort() { return _serverInfo.collect_port; }
	static int GetCollectIOCPWorkerCreate() { return _serverInfo.collect_workerCreateCnt; }
	static int GetCollectIOCPWorkerRunning() { return _serverInfo.collect_workerRunningCnt; }
	static WORD GetCollectSessionMax() { return _serverInfo.collect_sessionMax; }
	static const wchar_t* GetMonitorServerIP() { return _serverInfo.monitor_ip; }
	static int GetMonitorServerPort() { return _serverInfo.monitor_port; }
	static int GetMonitorIOCPWorkerCreate() { return _serverInfo.monitor_workerCreateCnt; }
	static int GetMonitorIOCPWorkerRunning() { return _serverInfo.monitor_workerRunningCnt; }
	static WORD GetMonitorSessionMax() { return _serverInfo.monitor_sessionMax; }
	static BYTE GetPacketCode() { return _serverInfo.packetCode; }
	static BYTE GetPacketKey() { return _serverInfo.packetKey; }
	static const wchar_t* GetLoginKey() { return _serverInfo.loginKey; }
	static int GetLogLevel() { return _serverInfo.logLevel; }
	static const wchar_t* GetLogPath() { return _serverInfo.logPath; }
	static const wchar_t* GetDatabaseIP() { return _databaseInfo.ip; }
	static int GetDatabasePort() { return _databaseInfo.port; }
	static const wchar_t* GetDatabaseUser() { return _databaseInfo.user; }
	static const wchar_t* GetDatabasePassword() { return _databaseInfo.passwd; }
	static const wchar_t* GetDatabaseSchema() { return _databaseInfo.schema; }
private:
	static SERVER_INFO _serverInfo;
	static DATABASE_INFO _databaseInfo;
};
