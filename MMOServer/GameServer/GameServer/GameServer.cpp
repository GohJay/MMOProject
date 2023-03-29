#include "stdafx.h"
#include "GameServer.h"
#include "Packet.h"
#include "../../Common/CommonProtocol.h"

using namespace Jay;

GameServer::GameServer(MainServer* subject) : _subject(subject), _stopSignal(false)
{
	_subject->AttachContent(this, CONTENT_ID_GAME, FRAME_INTERVAL_GAME);
	_managementThread = std::thread(&GameServer::ManagementThread, this);
}
GameServer::~GameServer()
{
	_stopSignal = true;
	_managementThread.join();
}
int GameServer::GetPlayerCount()
{
	return _playerTable.size();
}
int GameServer::GetFPS()
{
	return _oldFPS;
}
void GameServer::OnUpdate()
{
	_curFPS++;
}
void GameServer::OnClientJoin(DWORD64 sessionID)
{
}
void GameServer::OnClientLeave(DWORD64 sessionID)
{
	_playerTable.erase(sessionID);
}
void GameServer::OnContentEnter(DWORD64 sessionID, WPARAM wParam, LPARAM lParam)
{
	_playerTable.insert(sessionID);
}
void GameServer::OnContentExit(DWORD64 sessionID)
{
	_playerTable.erase(sessionID);
}
void GameServer::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 수신 받은 메시지 처리
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		_subject->Disconnect(sessionID);
}
void GameServer::ManagementThread()
{
	while (!_stopSignal)
	{
		Sleep(1000);
		UpdateFPS();
	}
}
void GameServer::UpdateFPS()
{
	_oldFPS.exchange(_curFPS.exchange(0));
}
bool GameServer::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// 수신 메시지 타입에 따른 분기 처리
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_CS_GAME_REQ_ECHO:
		return PacketProc_GameEcho(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		return true;
	default:
		break;
	}
	return false;
}
bool GameServer::PacketProc_GameEcho(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// Packet Deserialize
	//--------------------------------------------------------------------
	INT64 accountNo;
	LONGLONG sendTick;
	(*packet) >> accountNo;
	(*packet) >> sendTick;

	//--------------------------------------------------------------------
	// Packet Serialize
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	Packet::MakeGameEcho(resPacket, accountNo, sendTick);
	_subject->SendPacket(sessionID, resPacket);

	NetPacket::Free(resPacket);
	return true;
}
