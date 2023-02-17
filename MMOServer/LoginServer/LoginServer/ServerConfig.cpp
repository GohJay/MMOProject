#include "stdafx.h"
#include "ServerConfig.h"
#pragma comment(lib, "../../Lib/TextParser/lib64/TextParser.lib")

SERVER_INFO ServerConfig::_serverInfo;
SERVICE_INFO ServerConfig::_serviceInfo;
DATABASE_INFO ServerConfig::_databaseInfo;

bool ServerConfig::LoadFile(const wchar_t* filename)
{
	Jay::ConfigParser confParser;
	if (!confParser.LoadFile(L"Config.cnf"))
		return false;

	confParser.GetValue(L"SERVER", L"IP", _serverInfo.ip);
	confParser.GetValue(L"SERVER", L"PORT", &_serverInfo.port);
	confParser.GetValue(L"SERVER", L"IOCP_WORKER_CREATE", &_serverInfo.workerCreateCnt);
	confParser.GetValue(L"SERVER", L"IOCP_WORKER_RUNNING", &_serverInfo.workerRunningCnt);
	confParser.GetValue(L"SERVER", L"SESSION_MAX", (int*)&_serverInfo.sessionMax);
	confParser.GetValue(L"SERVER", L"USER_MAX", (int*)&_serverInfo.userMax);
	confParser.GetValue(L"SERVER", L"PACKET_CODE", (int*)&_serverInfo.packetCode);
	confParser.GetValue(L"SERVER", L"PACKET_KEY", (int*)&_serverInfo.packetKey);
	confParser.GetValue(L"SERVER", L"LOG_LEVEL", &_serverInfo.logLevel);
	confParser.GetValue(L"SERVER", L"LOG_PATH", _serverInfo.logPath);
	confParser.GetValue(L"SERVICE", L"TIMEOUT_SEC", &_serviceInfo.timeoutSec);
	confParser.GetValue(L"SERVICE", L"GAMESERVER_IP", _serviceInfo.gameserver_ip);
	confParser.GetValue(L"SERVICE", L"GAMESERVER_PORT", &_serviceInfo.gameserver_port);
	confParser.GetValue(L"SERVICE", L"CHATSERVER_IP", _serviceInfo.chatserver_ip);
	confParser.GetValue(L"SERVICE", L"CHATSERVER_PORT", &_serviceInfo.chatserver_port);
	confParser.GetValue(L"DATABASE", L"IP", _databaseInfo.ip);
	confParser.GetValue(L"DATABASE", L"PORT", &_databaseInfo.port);
	confParser.GetValue(L"DATABASE", L"USER", _databaseInfo.user);
	confParser.GetValue(L"DATABASE", L"PASSWORD", _databaseInfo.passwd);
	confParser.GetValue(L"DATABASE", L"SCHEMA", _databaseInfo.schema);
	confParser.GetValue(L"DATABASE", L"REDIS_IP", _databaseInfo.redis_ip);
	confParser.GetValue(L"DATABASE", L"REDIS_PORT", &_databaseInfo.redis_port);
	confParser.GetValue(L"DATABASE", L"REDIS_TIMEOUT", &_databaseInfo.redis_timeout);
    return true;
}
