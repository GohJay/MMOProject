#include "stdafx.h"
#include "MonitorServer.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "../../Common/Logger.h"
#include "../../Common/CrashDump.h"
#include "../../Lib/Network/include/Error.h"
#pragma comment(lib, "../../Lib/Network/lib64/Network.lib")

using namespace Jay;

MonitorServer::MonitorServer()
{
}
MonitorServer::~MonitorServer()
{
}
void MonitorServer::Update(int serverNo, BYTE dataType, int dataValue, int timeStamp)
{
	if (_clientTable.empty())
		return;

	//--------------------------------------------------------------------
	// 오브젝트풀에서 직렬화 버퍼 할당
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();
	Packet::MakeDataUpdate(packet, serverNo, dataType, dataValue, timeStamp);

	//--------------------------------------------------------------------
	// 로그인된 모든 유저에게 데이터 갱신 메시지 보내기
	//--------------------------------------------------------------------
	_tableLock.Lock_Shared();
	SendBroadcast(packet);
	_tableLock.UnLock_Shared();

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
}
void MonitorServer::OnClientLeave(DWORD64 sessionID)
{
	_tableLock.Lock();
	auto iter = _clientTable.find(sessionID);
	if (iter != _clientTable.end())
		_clientTable.erase(iter);
	_tableLock.UnLock();
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
bool MonitorServer::ValidationKey(const char* loginSessionKey)
{
	//--------------------------------------------------------------------
	// 로그인 키 인증
	//--------------------------------------------------------------------
	char* key1 = (char*)loginSessionKey;
	char* key2 = (char*)ServerConfig::GetLoginKey();

	while (*key1 && *key2)
	{
		if (*key1 != *key2)
			break;

		key1 += sizeof(char);
		key2 += sizeof(wchar_t);
	}
	return (*key1 == *key2);
}
void MonitorServer::SendBroadcast(NetPacket* packet)
{
	for (auto iter = _clientTable.begin(); iter != _clientTable.end(); ++iter)
	{
		DWORD64 sessionID = *iter;
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
	// 로그인 키 인증
	//--------------------------------------------------------------------
	loginSessionKey[len] = '\0';
	BYTE status = ValidationKey(loginSessionKey) ? dfMONITOR_TOOL_LOGIN_OK : dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;

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

	if (status == dfMONITOR_TOOL_LOGIN_OK)
	{
		_tableLock.Lock();
		_clientTable.insert(sessionID);
		_tableLock.UnLock();
	}
	return true;
}
