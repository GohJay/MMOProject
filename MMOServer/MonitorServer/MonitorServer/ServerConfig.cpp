#include "stdafx.h"
#include "ServerConfig.h"
#include "../../Lib/TextParser/include/ConfigParser.h"
#pragma comment(lib, "../../Lib/TextParser/lib64/TextParser.lib")

SERVER_INFO ServerConfig::_serverInfo;
DATABASE_INFO ServerConfig::_databaseInfo;

bool ServerConfig::LoadFile(const wchar_t* filename)
{
	Jay::ConfigParser confParser;
	if (!confParser.LoadFile(L"Config.cnf"))
		return false;

	confParser.GetValue(L"SERVER", L"COLLECT_IP", _serverInfo.collect_ip);
	confParser.GetValue(L"SERVER", L"COLLECT_PORT", &_serverInfo.collect_port);
	confParser.GetValue(L"SERVER", L"COLLECT_IOCP_WORKER_CREATE", &_serverInfo.collect_workerCreateCnt);
	confParser.GetValue(L"SERVER", L"COLLECT_IOCP_WORKER_RUNNING", &_serverInfo.collect_workerRunningCnt);
	confParser.GetValue(L"SERVER", L"COLLECT_SESSION_MAX", (int*)&_serverInfo.collect_sessionMax);
	confParser.GetValue(L"SERVER", L"MONITOR_IP", _serverInfo.monitor_ip);
	confParser.GetValue(L"SERVER", L"MONITOR_PORT", &_serverInfo.monitor_port);
	confParser.GetValue(L"SERVER", L"MONITOR_IOCP_WORKER_CREATE", &_serverInfo.monitor_workerCreateCnt);
	confParser.GetValue(L"SERVER", L"MONITOR_IOCP_WORKER_RUNNING", &_serverInfo.monitor_workerRunningCnt);
	confParser.GetValue(L"SERVER", L"MONITOR_SESSION_MAX", (int*)&_serverInfo.monitor_sessionMax);
	confParser.GetValue(L"SERVER", L"PACKET_CODE", (int*)&_serverInfo.packetCode);
	confParser.GetValue(L"SERVER", L"PACKET_KEY", (int*)&_serverInfo.packetKey);
	confParser.GetValue(L"SERVER", L"LOGIN_KEY", _serverInfo.loginKey);
	confParser.GetValue(L"SERVER", L"LOG_LEVEL", &_serverInfo.logLevel);
	confParser.GetValue(L"SERVER", L"LOG_PATH", _serverInfo.logPath);
	confParser.GetValue(L"DATABASE", L"IP", _databaseInfo.ip);
	confParser.GetValue(L"DATABASE", L"PORT", &_databaseInfo.port);
	confParser.GetValue(L"DATABASE", L"USER", _databaseInfo.user);
	confParser.GetValue(L"DATABASE", L"PASSWORD", _databaseInfo.passwd);
	confParser.GetValue(L"DATABASE", L"SCHEMA", _databaseInfo.schema);
    return true;
}
