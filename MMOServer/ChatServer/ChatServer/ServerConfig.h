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
private:
	static SERVER_INFO _serverInfo;
	static SERVICE_INFO _serviceInfo;
};
