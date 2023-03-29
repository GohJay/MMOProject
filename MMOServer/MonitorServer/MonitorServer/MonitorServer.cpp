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
	// 오브젝트풀에서 직렬화 버퍼 할당
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// 로그인된 모든 유저에게 데이터 갱신 메시지 보내기
	//--------------------------------------------------------------------
	Packet::MakeDataUpdate(packet, serverNo, dataType, dataValue, timeStamp);
	SendBroadcast(packet);

	//--------------------------------------------------------------------
	// 오브젝트풀에 직렬화 버퍼 반납
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
	// 수신 받은 메시지 처리
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		Disconnect(sessionID);
}
void MonitorServer::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------
	// Network IO Error 로깅
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Monitor"
		, LOG_LEVEL_ERROR
		, L"func: %s, line: %d, error: %d, wParam: %llu, lParam: %llu"
		, funcname, linenum, errcode, wParam, lParam);

	//--------------------------------------------------------------------
	// Fatal Error 일 경우 크래시와 함께 메모리 덤프를 남긴다.
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
	// 로그인 키 인증
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
	// 수신 메시지 타입에 따른 분기 처리
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
	// 로그인 메시지 처리
	//--------------------------------------------------------------------
	char loginSessionKey[33];
	int len = sizeof(loginSessionKey) - 1;
	if (packet->GetData(loginSessionKey, len) != len)
		return false;

	//--------------------------------------------------------------------
	// 중복 로그인 확인
	//--------------------------------------------------------------------
	USER* user;
	FindUser(sessionID, &user);
	if (user->login)
		return false;

	//--------------------------------------------------------------------
	// 로그인 키 인증
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
	// 오브젝트풀에서 직렬화 버퍼 할당
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// 해당 유저에게 로그인 응답 메시지 보내기
	//--------------------------------------------------------------------
	Packet::MakeLogin(resPacket, status);
	NetServer::SendPacket(sessionID, resPacket);

	//--------------------------------------------------------------------
	// 오브젝트풀에 직렬화 버퍼 반납
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
