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

LoginServer::LoginServer()
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
int LoginServer::GetAuthTPS()
{
	return _oldAuthTPS;
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
}
void LoginServer::OnClientLeave(DWORD64 sessionID)
{
}
void LoginServer::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------
	// Network IO Error �α�
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Login"
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
	_serviceMode = false;
	_stopSignal = false;

	//--------------------------------------------------------------------
	// Redis Connect
	//--------------------------------------------------------------------
	std::string redisIP;
	UnicodeToString(ServerConfig::GetRedisIP(), redisIP);
	_memorydb.connect(redisIP, ServerConfig::GetRedisPort());

	//--------------------------------------------------------------------
	// Database Init
	//--------------------------------------------------------------------
	_accountdb.SetProperty(ServerConfig::GetDatabaseIP()
		, ServerConfig::GetDatabasePort()
		, ServerConfig::GetDatabaseUser()
		, ServerConfig::GetDatabasePassword()
		, ServerConfig::GetDatabaseSchema());

	GetWhiteIPList();

	//--------------------------------------------------------------------
	// Thread Begin
	//--------------------------------------------------------------------
	_managementThread = std::thread(&LoginServer::ManagementThread, this);
	return true;
}
void LoginServer::Release()
{
	//--------------------------------------------------------------------
	// Thread End
	//--------------------------------------------------------------------
	_stopSignal = true;
	_managementThread.join();

	//--------------------------------------------------------------------
	// Redis Disconnect
	//--------------------------------------------------------------------
	_memorydb.disconnect();
}
void LoginServer::ManagementThread()
{
	while (!_stopSignal)
	{
		Sleep(1000);
		UpdateTPS();
	}
}
void LoginServer::UpdateTPS()
{
	_oldAuthTPS.exchange(_curAuthTPS.exchange(0));
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
void LoginServer::GetWhiteIPList()
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
void LoginServer::GetAccountInfo(INT64 accountNo, char* sessionKey, BYTE& status, std::wstring& userID, std::wstring& userNick)
{
	INT64 db_accountNo;
	char db_sessionKey[64];
	int db_status;

	sql::ResultSet* res = nullptr;
	_accountdb.Execute(L"START TRANSACTION;");
	
	do
	{
		//--------------------------------------------------------------------
		// DB ���� ȸ�� ���� ��ȸ
		//--------------------------------------------------------------------
		res = _accountdb.ExecuteQuery(L"SELECT\
`a`.`accountno` AS `accountno`,\
`a`.`userid` AS `userid`,\
`a`.`usernick` AS `usernick`,\
`b`.`sessionkey` AS `sessionkey`,\
`c`.`status` AS `status`\
FROM ((`account` `a`\
LEFT JOIN `sessionkey` `b` ON((`a`.`accountno` = `b`.`accountno`)))\
LEFT JOIN `status` `c` ON((`a`.`accountno` = `c`.`accountno`)))\
WHERE a.`accountno` = %lld FOR UPDATE;", accountNo);

		if (!res->next())
			break;

		db_accountNo = res->getInt64(1);
		MultiByteToWString(res->getString(2).c_str(), userID);
		MultiByteToWString(res->getString(3).c_str(), userNick);
		memmove(db_sessionKey, res->getString(4).c_str(), res->getString(4).length());
		db_status = res->getInt(5);

		//--------------------------------------------------------------------
		// ���� Ű ���� ( ���� Ŭ���̾�Ʈ�� ��� ���� ���� )
		//--------------------------------------------------------------------
		if (accountNo > 999999 && strncmp(sessionKey, db_sessionKey, sizeof(db_sessionKey)) != 0)
			break;

		//--------------------------------------------------------------------
		// �ߺ� �α��� ���� ����
		//--------------------------------------------------------------------
		if (db_status == ACCOUNT_STATUS_LOGIN_ING)
		{
			status = dfLOGIN_STATUS_NONE;
			break;
		}
		else if (db_status == ACCOUNT_STATUS_LOGIN_DONE)
		{
			status = dfLOGIN_STATUS_FAIL;
			break;
		}
		else if (db_status == ACCOUNT_STATUS_GAME_PLAY)
		{
			status = dfLOGIN_STATUS_GAME;
			break;
		}

		//--------------------------------------------------------------------
		// DB �� �α��� ���� ������Ʈ ( OFFLINE -> LOGIN_ING )
		//--------------------------------------------------------------------
		_accountdb.ExecuteUpdate(L"UPDATE status SET status = %d WHERE accountno = %lld;"
			, ACCOUNT_STATUS_LOGIN_ING
			, accountNo);

		status = dfLOGIN_STATUS_OK;
	} while (0);

	_accountdb.Execute(L"COMMIT;");
	_accountdb.ClearQuery(res);
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
	// DB���� ȸ�� ���� ��ȸ �� ����
	//--------------------------------------------------------------------
	BYTE status;
	std::wstring userID;
	std::wstring userNick;
	GetAccountInfo(accountNo, sessionKey, status, userID, userNick);

	if (status == dfLOGIN_STATUS_OK)
	{
		//--------------------------------------------------------------------
		// Redis �� ���� ��ū ���� (Key: AccountNo, Value: SessionKey)
		//--------------------------------------------------------------------
		std::string key(std::to_string(accountNo));
		std::string token(sessionKey, sizeof(sessionKey));
		_memorydb.setex(key + "_chat", ServerConfig::GetRedisTimeoutSec(), token);
		_memorydb.setex(key + "_game", ServerConfig::GetRedisTimeoutSec(), token);
		_memorydb.sync_commit();

		//--------------------------------------------------------------------
		// DB �� �α��� ���� ������Ʈ ( LOGIN_ING -> LOGIN_DONE )
		//--------------------------------------------------------------------
		_accountdb.ExecuteUpdate(L"UPDATE status SET status = %d WHERE accountno = %lld;"
			, ACCOUNT_STATUS_LOGIN_DONE
			, accountNo);
	}

	//--------------------------------------------------------------------
	// �ش� �������� �α��� ���� �޽��� ������
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	Packet::MakeLogin(resPacket
		, accountNo
		, status
		, &userID[0]
		, &userNick[0]
		, (WCHAR*)ServerConfig::GetGameServerIP()
		, ServerConfig::GetGameServerPort()
		, (WCHAR*)ServerConfig::GetChatServerIP()
		, ServerConfig::GetChatServerPort());
	SendPacket(sessionID, resPacket);

	NetPacket::Free(resPacket);

	_curAuthTPS++;
	return true;
}
