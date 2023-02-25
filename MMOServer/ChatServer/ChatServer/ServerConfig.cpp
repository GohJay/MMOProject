#include "stdafx.h"
#include "ServerConfig.h"
#include "../../Lib/TextParser/include/ConfigParser.h"
#pragma comment(lib, "../../Lib/TextParser/lib64/TextParser.lib")

SERVER ServerConfig::_server;
CLIENT ServerConfig::_client;
SYSTEM ServerConfig::_system;
DATABASE ServerConfig::_database;

bool ServerConfig::LoadFile(const wchar_t* filename)
{
	Jay::ConfigParser confParser;
	if (!confParser.LoadFile(L"Config.cnf"))
		return false;

	confParser.GetValue(L"SERVER", L"IP", _server.ip);
	confParser.GetValue(L"SERVER", L"PORT", &_server.port);
	confParser.GetValue(L"SERVER", L"IOCP_WORKER_CREATE", &_server.workerCreateCnt);
	confParser.GetValue(L"SERVER", L"IOCP_WORKER_RUNNING", &_server.workerRunningCnt);
	confParser.GetValue(L"SERVER", L"SESSION_MAX", (int*)&_server.sessionMax);
	confParser.GetValue(L"SERVER", L"USER_MAX", (int*)&_server.userMax);
	confParser.GetValue(L"SERVER", L"PACKET_CODE", (int*)&_server.packetCode);
	confParser.GetValue(L"SERVER", L"PACKET_KEY", (int*)&_server.packetKey);
	confParser.GetValue(L"SERVER", L"TIMEOUT_SEC", &_server.timeoutSec);
	confParser.GetValue(L"CLIENT", L"IP", _client.ip);
	confParser.GetValue(L"CLIENT", L"PORT", &_client.port);
	confParser.GetValue(L"CLIENT", L"RECONNECT", (int*)&_client.reconnect);
	confParser.GetValue(L"SYSTEM", L"LOG_LEVEL", &_system.logLevel);
	confParser.GetValue(L"SYSTEM", L"LOG_PATH", _system.logPath);
	confParser.GetValue(L"DATABASE", L"REDIS_IP", _database.redis_ip);
	confParser.GetValue(L"DATABASE", L"REDIS_PORT", &_database.redis_port);
    return true;
}
