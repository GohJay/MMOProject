#include "stdafx.h"
#include "ServerConfig.h"
#include "../../Lib/TextParser/include/ConfigParser.h"
#pragma comment(lib, "../../Lib/TextParser/lib64/TextParser.lib")

SERVER ServerConfig::_server;
SERVICE ServerConfig::_service;
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
	confParser.GetValue(L"SERVICE", L"GAMESERVER_IP", _service.gameserver_ip);
	confParser.GetValue(L"SERVICE", L"GAMESERVER_PORT", &_service.gameserver_port);
	confParser.GetValue(L"SERVICE", L"CHATSERVER_IP", _service.chatserver_ip);
	confParser.GetValue(L"SERVICE", L"CHATSERVER_PORT", &_service.chatserver_port);
	confParser.GetValue(L"SYSTEM", L"LOG_LEVEL", &_system.logLevel);
	confParser.GetValue(L"SYSTEM", L"LOG_PATH", _system.logPath);
	confParser.GetValue(L"DATABASE", L"IP", _database.ip);
	confParser.GetValue(L"DATABASE", L"PORT", &_database.port);
	confParser.GetValue(L"DATABASE", L"USER", _database.user);
	confParser.GetValue(L"DATABASE", L"PASSWORD", _database.passwd);
	confParser.GetValue(L"DATABASE", L"SCHEMA", _database.schema);
	confParser.GetValue(L"DATABASE", L"REDIS_IP", _database.redis_ip);
	confParser.GetValue(L"DATABASE", L"REDIS_PORT", &_database.redis_port);
	confParser.GetValue(L"DATABASE", L"REDIS_TIMEOUT_SEC", &_database.redis_timeout_sec);
    return true;
}
