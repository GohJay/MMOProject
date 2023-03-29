#include "stdafx.h"
#include "ServerConfig.h"
#include "../../Lib/TextParser/include/ConfigParser.h"
#pragma comment(lib, "../../Lib/TextParser/lib64/TextParser.lib")

SERVER ServerConfig::_server;
DATABASE ServerConfig::_database;
SERVICE ServerConfig::_service;
SYSTEM ServerConfig::_system;

bool ServerConfig::LoadFile(const wchar_t* filename)
{
	Jay::ConfigParser confParser;
	if (!confParser.LoadFile(L"Config.cnf"))
		return false;

	confParser.GetValue(L"SERVER", L"COLLECT_IP", _server.collectIP);
	confParser.GetValue(L"SERVER", L"COLLECT_PORT", &_server.collectPort);
	confParser.GetValue(L"SERVER", L"COLLECT_IOCP_WORKER_CREATE", &_server.collectWorkerCreateCnt);
	confParser.GetValue(L"SERVER", L"COLLECT_IOCP_WORKER_RUNNING", &_server.collectWorkerRunningCnt);
	confParser.GetValue(L"SERVER", L"COLLECT_SESSION_MAX", (int*)&_server.collectSessionMax);
	confParser.GetValue(L"SERVER", L"MONITOR_IP", _server.monitorIP);
	confParser.GetValue(L"SERVER", L"MONITOR_PORT", &_server.monitorPort);
	confParser.GetValue(L"SERVER", L"MONITOR_IOCP_WORKER_CREATE", &_server.monitorWorkerCreateCnt);
	confParser.GetValue(L"SERVER", L"MONITOR_IOCP_WORKER_RUNNING", &_server.monitorWorkerRunningCnt);
	confParser.GetValue(L"SERVER", L"MONITOR_SESSION_MAX", (int*)&_server.monitorSessionMax);
	confParser.GetValue(L"SERVER", L"PACKET_CODE", (int*)&_server.packetCode);
	confParser.GetValue(L"SERVER", L"PACKET_KEY", (int*)&_server.packetKey);
	confParser.GetValue(L"SERVICE", L"LOGIN_KEY", _service.loginKey);
	confParser.GetValue(L"SYSTEM", L"LOG_LEVEL", &_system.logLevel);
	confParser.GetValue(L"SYSTEM", L"LOG_PATH", _system.logPath);
	confParser.GetValue(L"DATABASE", L"IP", _database.ip);
	confParser.GetValue(L"DATABASE", L"PORT", &_database.port);
	confParser.GetValue(L"DATABASE", L"USER", _database.user);
	confParser.GetValue(L"DATABASE", L"PASSWORD", _database.passwd);
	confParser.GetValue(L"DATABASE", L"SCHEMA", _database.schema);
    return true;
}
