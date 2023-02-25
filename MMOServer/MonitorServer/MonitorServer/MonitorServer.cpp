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
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();
	Packet::MakeDataUpdate(packet, serverNo, dataType, dataValue, timeStamp);

	//--------------------------------------------------------------------
	// �α��ε� ��� �������� ������ ���� �޽��� ������
	//--------------------------------------------------------------------
	_tableLock.Lock_Shared();
	SendBroadcast(packet);
	_tableLock.UnLock_Shared();

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
bool MonitorServer::ValidationKey(const char* loginSessionKey)
{
	//--------------------------------------------------------------------
	// �α��� Ű ����
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
	// �α��� Ű ����
	//--------------------------------------------------------------------
	loginSessionKey[len] = '\0';
	BYTE status = ValidationKey(loginSessionKey) ? dfMONITOR_TOOL_LOGIN_OK : dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;

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

	if (status == dfMONITOR_TOOL_LOGIN_OK)
	{
		_tableLock.Lock();
		_clientTable.insert(sessionID);
		_tableLock.UnLock();
	}
	return true;
}
