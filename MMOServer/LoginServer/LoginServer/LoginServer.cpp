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
		return false;

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
}
void LoginServer::FetchWhiteIPList()
{
	std::wstring ip;
	sql::ResultSet* res;

	//--------------------------------------------------------------------
	// DB ���� White IP ��� ��������
	//--------------------------------------------------------------------
	res = _accountdb.ExecuteQuery(L"SELECT ip FROM whiteip LIMIT 10");
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

	INT64 db_accountNo;
	char db_sessionKey[64];
	int db_status;
	int db_timeSec;
	std::wstring db_userID;
	std::wstring db_userNick;
	db_userID.reserve(20);
	db_userNick.reserve(20);

	BYTE status;
	sql::ResultSet* res;

	//--------------------------------------------------------------------
	// DB ���� ���� ���� ��ȸ
	//--------------------------------------------------------------------
	res = _accountdb.ExecuteQuery(L"SELECT accountno, userid, usernick, sessionkey, status FROM v_account WHERE accountno = %lld", accountNo);
	if (res->next())
	{
		db_accountNo = res->getInt64(1);
		MultiByteToWString(res->getString(2).c_str(), db_userID);
		MultiByteToWString(res->getString(3).c_str(), db_userNick);
		strncpy_s(db_sessionKey, res->getString(4).c_str(), sizeof(db_sessionKey));
		db_status = res->getInt(5);

		status = dfLOGIN_STATUS_OK;
	}
	else
	{
		status = dfLOGIN_STATUS_FAIL;
	}
	_accountdb.ClearQuery(res);

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
