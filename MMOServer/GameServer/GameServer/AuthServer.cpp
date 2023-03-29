#include "stdafx.h"
#include "AuthServer.h"
#include "Packet.h"
#include "../../Common/CommonProtocol.h"

using namespace Jay;

AuthServer::AuthServer(MainServer* subject) : _subject(subject), _stopSignal(false)
{
	_subject->AttachContent(this, CONTENT_ID_AUTH, FRAME_INTERVAL_AUTH, true);
	_managementThread = std::thread(&AuthServer::ManagementThread, this);
}
AuthServer::~AuthServer()
{
	_stopSignal = true;
	_managementThread.join();
}
int AuthServer::GetPlayerCount()
{
	return _playerTable.size();
}
int AuthServer::GetFPS()
{
	return _oldFPS;
}
void AuthServer::OnUpdate()
{
	_curFPS++;
}
void AuthServer::OnClientJoin(DWORD64 sessionID)
{
}
void AuthServer::OnClientLeave(DWORD64 sessionID)
{
	_playerTable.erase(sessionID);
}
void AuthServer::OnContentEnter(DWORD64 sessionID, WPARAM wParam, LPARAM lParam)
{
	_playerTable.insert(sessionID);
}
void AuthServer::OnContentExit(DWORD64 sessionID)
{
	_playerTable.erase(sessionID);
}
void AuthServer::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 수신 받은 메시지 처리
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		_subject->Disconnect(sessionID);
}
void AuthServer::ManagementThread()
{
	while (!_stopSignal)
	{
		Sleep(1000);
		UpdateFPS();
	}
}
void AuthServer::UpdateFPS()
{
	_oldFPS.exchange(_curFPS.exchange(0));
}
bool AuthServer::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// 수신 메시지 타입에 따른 분기 처리
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_CS_GAME_REQ_LOGIN:
		return PacketProc_GameLogin(sessionID, packet);
	default:
		break;
	}
	return false;
}
bool AuthServer::PacketProc_GameLogin(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// Packet Deserialize
	//--------------------------------------------------------------------
	INT64 accountNo;
	char sessionKey[64];
	int version;
	(*packet) >> accountNo;
	if (packet->GetData(sessionKey, sizeof(sessionKey)) != sizeof(sessionKey))
		return false;
	(*packet) >> version;

	//--------------------------------------------------------------------
	// Move Content To GameThread
	//--------------------------------------------------------------------
	_subject->MoveContent(sessionID, CONTENT_ID_GAME, NULL, NULL);

	//--------------------------------------------------------------------
	// Packet Serialize
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	Packet::MakeGameLogin(resPacket, true, accountNo);
	_subject->SendPacket(sessionID, resPacket);

	NetPacket::Free(resPacket);
	return true;
}
