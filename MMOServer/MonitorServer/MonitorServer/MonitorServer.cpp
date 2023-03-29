#include "stdafx.h"
#include "MonitorServer.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "../../Common/Logger.h"
#include "../../Common/CrashDump.h"
#include "../../Lib/Network/include/Error.h"
#pragma comment(lib, "../../Lib/Network/lib64/Network.lib")

using namespace Jay;

MonitorServer::MonitorServer() : _stopSignal(false)
{
}
MonitorServer::~MonitorServer()
{
}
bool MonitorServer::Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec, bool nagle)
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
void MonitorServer::Stop()
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
void MonitorServer::Update(int serverNo, BYTE dataType, int dataValue, int timeStamp)
{
	if (_userMap.empty())
		return;

	//--------------------------------------------------------------------
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// �α��ε� ��� �������� ������ ���� �޽��� ������
	//--------------------------------------------------------------------
	Packet::MakeDataUpdate(packet, serverNo, dataType, dataValue, timeStamp);
	SendBroadcast(packet);

	//--------------------------------------------------------------------
	// ������ƮǮ�� ����ȭ ���� �ݳ�
	//--------------------------------------------------------------------
	NetPacket::Free(packet);
}
bool MonitorServer::OnConnectionRequest(const wchar_t* ipaddress, int port)
{
	return true;
}
void MonitorServer::OnClientJoin(DWORD64 sessionID)
{
	NewUser(sessionID);
}
void MonitorServer::OnClientLeave(DWORD64 sessionID)
{
	DeleteUser(sessionID);
}
void MonitorServer::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ���� ���� �޽��� ó��
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		Disconnect(sessionID);
}
void MonitorServer::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------
	// Network IO Error �α�
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Monitor"
		, LOG_LEVEL_ERROR
		, L"func: %s, line: %d, error: %d, wParam: %llu, lParam: %llu"
		, funcname, linenum, errcode, wParam, lParam);

	//--------------------------------------------------------------------
	// Fatal Error �� ��� ũ���ÿ� �Բ� �޸� ������ �����.
	//--------------------------------------------------------------------
	if (errcode >= NET_FATAL_INVALID_SIZE)
		CrashDump::Crash();
}
bool MonitorServer::Initial()
{
	//--------------------------------------------------------------------
	// Thread Begin
	//--------------------------------------------------------------------
	_managementThread = std::thread(&MonitorServer::ManagementThread, this);
	return true;
}
void MonitorServer::Release()
{
	//--------------------------------------------------------------------
	// Thread End
	//--------------------------------------------------------------------
	_stopSignal = true;
	_managementThread.join();
}
void MonitorServer::ManagementThread()
{
	while (!_stopSignal)
	{
		TimeoutProc();
		Sleep(5000);
	}
}
void MonitorServer::TimeoutProc()
{
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	DWORD currentTime = timeGetTime();
	for (auto iter = _userMap.begin(); iter != _userMap.end(); ++iter)
	{
		SESSION_ID sessionID = iter->first;
		USER* user = iter->second;
		if (user->login)
			continue;

		if (currentTime - user->connectionTime >= dfNETWORK_NON_LOGIN_USER_TIMEOUT)
			NetServer::Disconnect(sessionID);
	}
}
MonitorServer::USER* MonitorServer::NewUser(DWORD64 sessionID)
{
	USER* user = new USER;
	user->login = false;
	user->connectionTime = timeGetTime();

	_mapLock.Lock();
	_userMap.insert({ sessionID, user });
	_mapLock.UnLock();

	return user;
}
void MonitorServer::DeleteUser(DWORD64 sessionID)
{
	USER* user;

	_mapLock.Lock();
	auto iter = _userMap.find(sessionID);
	user = iter->second;
	_userMap.erase(iter);
	_mapLock.UnLock();

	delete user;
}
bool MonitorServer::FindUser(DWORD64 sessionID, USER** user)
{
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	auto iter = _userMap.find(sessionID);
	if (iter != _userMap.end())
	{
		*user = iter->second;
		return true;
	}
	return false;
}
bool MonitorServer::ValidationKey(const char* loginSessionKey)
{
	//--------------------------------------------------------------------
	// �α��� Ű ����
	//--------------------------------------------------------------------
	char* key1 = (char*)loginSessionKey;
	char* key2 = (char*)ServerConfig::GetLoginKey();

	while (*key1 || *key2)
	{
		if (*key1 != *key2)
			return false;

		key1 += sizeof(char);
		key2 += sizeof(wchar_t);
	}
	return true;
}
void MonitorServer::SendBroadcast(NetPacket* packet)
{
	DWORD64 sessionID;
	USER* user;
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	for (auto iter = _userMap.begin(); iter != _userMap.end(); ++iter)
	{
		sessionID = iter->first;
		user = iter->second;

		if (user->login)
			NetServer::SendPacket(sessionID, packet);
	}
}
bool MonitorServer::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// ���� �޽��� Ÿ�Կ� ���� �б� ó��
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		return PacketProc_Login(sessionID, packet);
	default:
		break;
	}
	return false;
}
bool MonitorServer::PacketProc_Login(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// �α��� �޽��� ó��
	//--------------------------------------------------------------------
	char loginSessionKey[33];
	int len = sizeof(loginSessionKey) - 1;
	if (packet->GetData(loginSessionKey, len) != len)
		return false;

	//--------------------------------------------------------------------
	// �ߺ� �α��� Ȯ��
	//--------------------------------------------------------------------
	USER* user;
	FindUser(sessionID, &user);
	if (user->login)
		return false;

	//--------------------------------------------------------------------
	// �α��� Ű ����
	//--------------------------------------------------------------------
	BYTE status;
	loginSessionKey[len] = '\0';
	if (ValidationKey(loginSessionKey))
	{
		user->login = true;
		status = dfMONITOR_TOOL_LOGIN_OK;
	}
	else
		status = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;

	//--------------------------------------------------------------------
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// �ش� �������� �α��� ���� �޽��� ������
	//--------------------------------------------------------------------
	Packet::MakeLogin(resPacket, status);
	NetServer::SendPacket(sessionID, resPacket);

	//--------------------------------------------------------------------
	// ������ƮǮ�� ����ȭ ���� �ݳ�
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
