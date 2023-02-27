#pragma once
#include "Define.h"

class ServerConfig
{
public:
	static bool LoadFile(const wchar_t* filename);
	static const wchar_t* GetLoginServerIP() { return _server.ip; }
	static int GetLoginServerPort() { return _server.port; }
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
	static const wchar_t* GetGameServerIP() { return _service.gameserver_ip; }
	static int GetGameServerPort() { return _service.gameserver_port; }
	static const wchar_t* GetChatServerIP() { return _service.chatserver_ip; }
	static int GetChatServerPort() { return _service.chatserver_port; }
	static int GetLogLevel() { return _system.logLevel; }
	static const wchar_t* GetLogPath() { return _system.logPath; }
	static const wchar_t* GetDatabaseIP() { return _database.ip; }
	static int GetDatabasePort() { return _database.port; }
	static const wchar_t* GetDatabaseUser() { return _database.user; }
	static const wchar_t* GetDatabasePassword() { return _database.passwd; }
	static const wchar_t* GetDatabaseSchema() { return _database.schema; }
	static const wchar_t* GetRedisIP() { return _database.redis_ip; }
	static int GetRedisPort() { return _database.redis_port; }
	static int GetRedisTimeoutSec() { return _database.redis_timeout_sec; }
private:
	static SERVER _server;
	static CLIENT _client;
	static SERVICE _service;
	static SYSTEM _system;
	static DATABASE _database;
};
