#pragma once
#include "Define.h"

class ServerConfig
{
public:
	static bool LoadFile(const wchar_t* filename);
	static const wchar_t* GetCollectServerIP() { return _server.collectIP; }
	static int GetCollectServerPort() { return _server.collectPort; }
	static int GetCollectIOCPWorkerCreate() { return _server.collectWorkerCreateCnt; }
	static int GetCollectIOCPWorkerRunning() { return _server.collectWorkerRunningCnt; }
	static WORD GetCollectSessionMax() { return _server.collectSessionMax; }
	static const wchar_t* GetMonitorServerIP() { return _server.monitorIP; }
	static int GetMonitorServerPort() { return _server.monitorPort; }
	static int GetMonitorIOCPWorkerCreate() { return _server.monitorWorkerCreateCnt; }
	static int GetMonitorIOCPWorkerRunning() { return _server.monitorWorkerRunningCnt; }
	static WORD GetMonitorSessionMax() { return _server.monitorSessionMax; }
	static BYTE GetPacketCode() { return _server.packetCode; }
	static BYTE GetPacketKey() { return _server.packetKey; }
	static const wchar_t* GetLoginKey() { return _service.loginKey; }
	static int GetLogLevel() { return _system.logLevel; }
	static const wchar_t* GetLogPath() { return _system.logPath; }
	static const wchar_t* GetDatabaseIP() { return _database.ip; }
	static int GetDatabasePort() { return _database.port; }
	static const wchar_t* GetDatabaseUser() { return _database.user; }
	static const wchar_t* GetDatabasePassword() { return _database.passwd; }
	static const wchar_t* GetDatabaseSchema() { return _database.schema; }
private:
	static SERVER _server;
	static DATABASE _database;
	static SERVICE _service;
	static SYSTEM _system;
};
