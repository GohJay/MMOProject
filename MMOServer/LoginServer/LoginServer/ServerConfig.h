#pragma once
#include "Define.h"
#include "../../Lib/TextParser/include/ConfigParser.h"

class ServerConfig
{
public:
	static bool LoadFile(const wchar_t* filename);
	static const wchar_t* GetServerIP() { return _serverInfo.ip; }
	static int GetServerPort() { return _serverInfo.port; }
	static int GetIOCPWorkerCreate() { return _serverInfo.workerCreateCnt; }
	static int GetIOCPWorkerRunning() { return _serverInfo.workerRunningCnt; }
	static WORD GetSessionMax() { return _serverInfo.sessionMax; }
	static WORD GetUserMax() { return _serverInfo.userMax; }
	static BYTE GetPacketCode() { return _serverInfo.packetCode; }
	static BYTE GetPacketKey() { return _serverInfo.packetKey; }
	static int GetLogLevel() { return _serverInfo.logLevel; }
	static const wchar_t* GetLogPath() { return _serverInfo.logPath; }
	static int GetSessionTimeoutSec() { return _serviceInfo.timeoutSec; }
	static const wchar_t* GetGameServerIP() { return _serviceInfo.gameserver_ip; }
	static int GetGameServerPort() { return _serviceInfo.gameserver_port; }
	static const wchar_t* GetChatServerIP() { return _serviceInfo.chatserver_ip; }
	static int GetChatServerPort() { return _serviceInfo.chatserver_port; }
	static const wchar_t* GetDatabaseIP() { return _databaseInfo.ip; }
	static int GetDatabasePort() { return _databaseInfo.port; }
	static const wchar_t* GetDatabaseUser() { return _databaseInfo.user; }
	static const wchar_t* GetDatabasePassword() { return _databaseInfo.passwd; }
	static const wchar_t* GetDatabaseSchema() { return _databaseInfo.schema; }
private:
	static SERVER_INFO _serverInfo;
	static SERVICE_INFO _serviceInfo;
	static DATABASE_INFO _databaseInfo;
};
