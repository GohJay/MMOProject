#pragma once
#include "Define.h"

class ServerConfig
{
public:
	static bool LoadFile(const wchar_t* filename);
	static const wchar_t* GetChatServerIP() { return _server.ip; }
	static int GetChatServerPort() { return _server.port; }
	static int GetIOCPWorkerCreate() { return _server.workerCreateCnt; }
	static int GetIOCPWorkerRunning() { return _server.workerRunningCnt; }
	static WORD GetSessionMax() { return _server.sessionMax; }
	static WORD GetUserMax() { return _server.userMax; }
	static BYTE GetPacketCode() { return _server.packetCode; }
	static BYTE GetPacketKey() { return _server.packetKey; }
	static int GetSessionTimeoutSec() { return _server.timeoutSec; }
	static const wchar_t* GetMonitorServerIP() { return _client.ip; }
	static int GetMonitorServerPort() { return _client.port; }
	static bool IsMonitorReconnect() { return _client.reconnect; }
	static int GetLogLevel() { return _system.logLevel; }
	static const wchar_t* GetLogPath() { return _system.logPath; }
	static const wchar_t* GetRedisIP() { return _database.redis_ip; }
	static int GetRedisPort() { return _database.redis_port; }
private:
	static SERVER _server;
	static CLIENT _client;
	static SYSTEM _system;
	static DATABASE _database;
};
