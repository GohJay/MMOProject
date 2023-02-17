#include "stdafx.h"
#include "LoginServer.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "../../Common/Logger.h"
#include "../../Common/CrashDump.h"
#include "../../Common/StringUtil.h"
#include "../../Lib/Network/include/Error.h"
#include "../../Lib/Network/include/NetException.h"
#pragma comment(lib, "../../Lib/Network/lib64/Network.lib")
#pragma comment(lib, "../../Lib/MySQL/lib64/vs14/mysqlcppconn-static.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/tacopie.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/cpp_redis.lib")

using namespace Jay;

LoginServer::LoginServer() : _serviceMode(false)
{
}
LoginServer::~LoginServer()
{
}
bool LoginServer::Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec, bool nagle)
{
	//--------------------------------------------------------------------
	// Initial
	//--------------------------------------------------------------------
	if (!Initial())
		return false;

	//--------------------------------------------------------------------
	// Network IO Start
	//--------------------------------------------------------------------
	if (!NetServer::Start(ipaddress, port, workerCreateCnt, workerRunningCnt, sessionMax, packetCode, packetKey, timeoutSec, nagle))
	{
		Release();
		return false;
	}
	
	return true;
}
void LoginServer::Stop()
{
	//--------------------------------------------------------------------
	// Network IO Stop
	//--------------------------------------------------------------------
	NetServer::Stop();

	//--------------------------------------------------------------------
	// Release
	//--------------------------------------------------------------------
	Release();
}
void LoginServer::SwitchServiceMode()
{
	_serviceMode = true;
}
bool LoginServer::OnConnectionRequest(const wchar_t* ipaddress, int port)
{
	if (!_serviceMode)
	{
		if (!CheckWhiteIP(ipaddress))
			return false;
	}
	return true;
}
void LoginServer::OnClientJoin(DWORD64 sessionID)
{	
}
void LoginServer::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ���� ���� �޽��� ó��
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		Disconnect(sessionID);

	NetPacket::Free(packet);
}
void LoginServer::OnClientLeave(DWORD64 sessionID)
{

}
void LoginServer::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------
	// Network IO Error �α�
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Chat"
		, LOG_LEVEL_ERROR
		, L"func: %s, line: %d, error: %d, wParam: %llu, lParam: %llu"
		, funcname, linenum, errcode, wParam, lParam);

	//--------------------------------------------------------------------
	// Fatal Error �� ��� ũ���ÿ� �Բ� �޸� ������ �����.
	//--------------------------------------------------------------------
	if (errcode >= NET_FATAL_INVALID_SIZE)
		CrashDump::Crash();
}
bool LoginServer::Initial()
{
	std::string redisIP;
	UnicodeToString(ServerConfig::GetRedisIP(), redisIP);
	_memorydb.connect(redisIP, ServerConfig::GetRedisPort());

	_accountdb.SetProperty(ServerConfig::GetDatabaseIP()
		, ServerConfig::GetDatabasePort()
		, ServerConfig::GetDatabaseUser()
		, ServerConfig::GetDatabasePassword()
		, ServerConfig::GetDatabaseSchema());

	FetchWhiteIPList();
	return true;
}
void LoginServer::Release()
{
	_memorydb.disconnect();
}
void LoginServer::FetchWhiteIPList()
{
	//--------------------------------------------------------------------
	// DB ���� White IP ��� ��������
	//--------------------------------------------------------------------
	std::wstring ip;
	sql::ResultSet* res = _accountdb.ExecuteQuery(L"SELECT ip FROM whiteip LIMIT 10;");
	while (res->next())
	{
		MultiByteToWString(res->getString(1).c_str(), ip);
		_whiteIPTable.insert(ip.c_str());
	}
	_accountdb.ClearQuery(res);
}
bool LoginServer::CheckWhiteIP(const wchar_t* ipaddress)
{
	//--------------------------------------------------------------------
	// White IP ��� Ȯ��
	//--------------------------------------------------------------------
	auto iter = _whiteIPTable.find(ipaddress);
	if (iter == _whiteIPTable.end())
		return false;

	return true;
}
bool LoginServer::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// ���� �޽��� Ÿ�Կ� ���� �б� ó��
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_CS_LOGIN_REQ_LOGIN:
		return PacketProc_Login(sessionID, packet);
	default:
		break;
	}
	return false;
}
bool LoginServer::PacketProc_Login(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// �α��� �޽��� ó��
	//--------------------------------------------------------------------	
	INT64 accountNo;
	char sessionKey[64];

	(*packet) >> accountNo;
	if (packet->GetData(sessionKey, sizeof(sessionKey)) != sizeof(sessionKey))
		return false;

	//--------------------------------------------------------------------
	// DB ���� ���� ���� ��ȸ
	//--------------------------------------------------------------------
	INT64 db_accountNo;
	char db_sessionKey[64];
	int db_status;
	int db_timeSec;
	std::wstring db_userID;
	std::wstring db_userNick;
	db_userID.reserve(20);
	db_userNick.reserve(20);

	sql::ResultSet* res = _accountdb.ExecuteQuery(L"SELECT\
`a`.`accountno` AS `accountno`,\
`a`.`userid` AS `userid`,\
`a`.`usernick` AS `usernick`,\
`b`.`sessionkey` AS `sessionkey`,\
`c`.`status` AS `status`\
FROM ((`account` `a`\
LEFT JOIN `sessionkey` `b` ON((`a`.`accountno` = `b`.`accountno`)))\
LEFT JOIN `status` `c` ON((`a`.`accountno` = `c`.`accountno`)))\
WHERE a.`accountno` = %lld;", accountNo);

	BYTE status;
	if (res->next())
	{
		db_accountNo = res->getInt64(1);
		MultiByteToWString(res->getString(2).c_str(), db_userID);
		MultiByteToWString(res->getString(3).c_str(), db_userNick);
		int size = sizeof(db_sessionKey);
		if (size > res->getString(4).length())
			size = res->getString(4).length();
		memmove(db_sessionKey, res->getString(4).c_str(), size);
		db_status = res->getInt(5);

		status = dfLOGIN_STATUS_OK;
	}
	else
	{
		status = dfLOGIN_STATUS_FAIL;
	}
	_accountdb.ClearQuery(res);

	//--------------------------------------------------------------------
	// Redis �� ���� ��ū ���� (Key: AccountNo, Value: SessionKey)
	//--------------------------------------------------------------------
	if (status == dfLOGIN_STATUS_OK)
	{
		std::string key(std::to_string(accountNo));
		std::string token(sessionKey, sizeof(sessionKey));
		_memorydb.setex(key, ServerConfig::GetRedisTimeout(), token);
		_memorydb.sync_commit();
	}

	//--------------------------------------------------------------------
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// �ش� �������� �α��� ���� �޽��� ������
	//--------------------------------------------------------------------
	Packet::MakeLogin(resPacket
		, accountNo
		, status
		, &db_userID[0]
		, &db_userNick[0]
		, (WCHAR*)ServerConfig::GetGameServerIP()
		, ServerConfig::GetGameServerPort()
		, (WCHAR*)ServerConfig::GetChatServerIP()
		, ServerConfig::GetChatServerPort());
	SendPacket(sessionID, resPacket);

	//--------------------------------------------------------------------
	// ������ƮǮ�� ����ȭ ���� �ݳ�
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
