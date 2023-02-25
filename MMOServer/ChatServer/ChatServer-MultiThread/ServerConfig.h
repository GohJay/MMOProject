#pragma once
#include "Define.h"

class ServerConfig
{
public:
	static bool LoadFile(const wchar_t* filename);
	static const wchar_t* GetServerIP() { return _server.ip; }
	static int GetServerPort() { return _server.port; }
	static int GetIOCPWorkerCreate() { return _server.workerCreateCnt; }
	static int GetIOCPWorkerRunning() { return _server.workerRunningCnt; }
	static WORD GetSessionMax() { return _server.sessionMax; }
	static WORD GetUserMax() { return _server.userMax; }
	static BYTE GetPacketCode() { return _server.packetCode; }
	static BYTE GetPacketKey() { return _server.packetKey; }
	static int GetSessionTimeoutSec() { return _server.timeoutSec; }
	static int GetLogLevel() { return _system.logLevel; }
	static const wchar_t* GetLogPath() { return _system.logPath; }
private:
	static SERVER _server;
	static SYSTEM _system;
};
