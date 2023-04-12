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
	// 수신 받은 메시지 처리
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
	// Network IO Error 로깅
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Login"
		, LOG_LEVEL_ERROR
		, L"func: %s, line: %d, error: %d, wParam: %llu, lParam: %llu"
		, funcname, linenum, errcode, wParam, lParam);

	//--------------------------------------------------------------------
	// Fatal Error 일 경우 크래시와 함께 메모리 덤프를 남긴다.
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
	// White IP 목록 확인
	//--------------------------------------------------------------------
	auto iter = _whiteIPTable.find(ipaddress);
	if (iter == _whiteIPTable.end())
		return false;

	return true;
}
void LoginServer::GetWhiteIPList()
{
	//--------------------------------------------------------------------
	// DB 에서 White IP 목록 가져오기
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
		// DB 에서 회원 정보 조회
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
		// 세션 키 검증 ( 더미 클라이언트일 경우 검증 제외 )
		//--------------------------------------------------------------------
		if (accountNo > 999999 && strncmp(sessionKey, db_sessionKey, sizeof(db_sessionKey)) != 0)
			break;

		//--------------------------------------------------------------------
		// 중복 로그인 여부 검증
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
		// DB 에 로그인 상태 업데이트 ( OFFLINE -> LOGIN_ING )
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
	// 수신 메시지 타입에 따른 분기 처리
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
	// 로그인 메시지 처리
	//--------------------------------------------------------------------
	INT64 accountNo;
	char sessionKey[64];

	(*packet) >> accountNo;
	if (packet->GetData(sessionKey, sizeof(sessionKey)) != sizeof(sessionKey))
		return false;

	//--------------------------------------------------------------------
	// DB에서 회원 정보 조회 및 검증
	//--------------------------------------------------------------------
	BYTE status;
	std::wstring userID;
	std::wstring userNick;
	GetAccountInfo(accountNo, sessionKey, status, userID, userNick);

	if (status == dfLOGIN_STATUS_OK)
	{
		//--------------------------------------------------------------------
		// Redis 에 세션 토큰 저장 (Key: AccountNo, Value: SessionKey)
		//--------------------------------------------------------------------
		std::string key(std::to_string(accountNo));
		std::string token(sessionKey, sizeof(sessionKey));
		_memorydb.setex(key + "_chat", ServerConfig::GetRedisTimeoutSec(), token);
		_memorydb.setex(key + "_game", ServerConfig::GetRedisTimeoutSec(), token);
		_memorydb.sync_commit();

		//--------------------------------------------------------------------
		// DB 에 로그인 상태 업데이트 ( LOGIN_ING -> LOGIN_DONE )
		//--------------------------------------------------------------------
		_accountdb.ExecuteUpdate(L"UPDATE status SET status = %d WHERE accountno = %lld;"
			, ACCOUNT_STATUS_LOGIN_DONE
			, accountNo);
	}

	//--------------------------------------------------------------------
	// 해당 유저에게 로그인 응답 메시지 보내기
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
