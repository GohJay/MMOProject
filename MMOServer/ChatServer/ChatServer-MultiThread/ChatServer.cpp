#include "stdafx.h"
#include "ChatServer.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "../../Common/Logger.h"
#include "../../Common/CrashDump.h"
#include "../../Lib/Network/include/Error.h"
#include "../../Lib/Network/include/NetException.h"
#pragma comment(lib, "../../Lib/Network/lib64/Network.lib")

using namespace Jay;

ChatServer::ChatServer() : _userPool(0), _playerPool(0)
{
}
ChatServer::~ChatServer()
{
}
bool ChatServer::Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec, bool nagle)
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
void ChatServer::Stop()
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
int ChatServer::GetUserCount()
{
	return _userMap.size();
}
int ChatServer::GetPlayerCount()
{
	return _playerMap.size();
}
int ChatServer::GetUseUserPool()
{
	return _userPool.GetUseCount();
}
int ChatServer::GetUsePlayerPool()
{
	return _playerPool.GetUseCount();
}
bool ChatServer::OnConnectionRequest(const wchar_t* ipaddress, int port)
{
	return true;
}
void ChatServer::OnClientJoin(DWORD64 sessionID)
{
	//--------------------------------------------------------------------
	// ���� ������ ���� Ȯ���Ͽ� Join ���� �Ǵ�
	//--------------------------------------------------------------------
	if (_playerMap.size() >= ServerConfig::GetUserMax())
	{
		Disconnect(sessionID);
		return;
	}

	//--------------------------------------------------------------------
	// ���� ����� ���ǿ� ���� ĳ���� �Ҵ�
	//--------------------------------------------------------------------
	NewUser(sessionID);
}
void ChatServer::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ���� ���� �޽��� ó��
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		Disconnect(sessionID);
}
void ChatServer::OnClientLeave(DWORD64 sessionID)
{
	//--------------------------------------------------------------------
	// ���� ������ ������ ĳ���� ����
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	if (user->login)
		DeletePlayer(user->accountNo);
	DeleteUser(sessionID);
}
void ChatServer::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
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
bool ChatServer::Initial()
{
	return true;
}
void ChatServer::Release()
{
	USER* user;
	for (auto iter = _userMap.begin(); iter != _userMap.end();)
	{
		user = iter->second;
		_userPool.Free(user);
		iter = _userMap.erase(iter);
	}

	PLAYER* player;
	for (auto iter = _playerMap.begin(); iter != _playerMap.end();)
	{
		player = iter->second;
		_playerPool.Free(player);
		iter = _playerMap.erase(iter);
	}
}
void ChatServer::NewUser(DWORD64 sessionID)
{
	USER* user = _userPool.Alloc();
	user->sessionID = sessionID;
	user->login = false;

	_mapLock.Lock();
	_userMap.insert({ sessionID, user });
	_mapLock.UnLock();
}
void ChatServer::DeleteUser(DWORD64 sessionID)
{
	_mapLock.Lock();
	auto iter = _userMap.find(sessionID);
	if (iter != _userMap.end())
	{
		USER* user = iter->second;
		_userMap.erase(iter);
		_userPool.Free(user);
	}
	_mapLock.UnLock();
}
USER* ChatServer::FindUser(INT64 sessionID)
{
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	USER* user;
	auto iter = _userMap.find(sessionID);
	if (iter != _userMap.end())
	{
		user = iter->second;
		return user;
	}
	return nullptr;
}
void ChatServer::NewPlayer(DWORD64 sessionID, INT64 accountNo)
{
	PLAYER* player = _playerPool.Alloc();
	player->sessionID = sessionID;
	player->accountNo = accountNo;
	player->sector.x = dfUNKNOWN_SECTOR;
	player->sector.y = dfUNKNOWN_SECTOR;

	_mapLock.Lock();
	_playerMap.insert({ accountNo, player });
	_mapLock.UnLock();
}
void ChatServer::DeletePlayer(INT64 accountNo)
{
	PLAYER* player;
	SRWLock* sectorLock;

	_mapLock.Lock();
	auto iter = _playerMap.find(accountNo);
	player = iter->second;
	_playerMap.erase(iter);
	_mapLock.UnLock();

	if (player->sector.x != dfUNKNOWN_SECTOR && player->sector.y != dfUNKNOWN_SECTOR)
	{
		sectorLock = &_sectorLockTable[player->sector.y][player->sector.x];
		sectorLock->Lock();
		RemovePlayer_Sector(player);
		sectorLock->UnLock();
	}
	_playerPool.Free(player);
}
PLAYER* ChatServer::FindPlayer(INT64 accountNo)
{
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	PLAYER* player;
	auto iter = _playerMap.find(accountNo);
	if (iter != _playerMap.end())
	{
		player = iter->second;
		return player;
	}
	return nullptr;
}
bool ChatServer::IsMovablePlayer(int sectorX, int sectorY)
{
	//--------------------------------------------------------------------
	// ���� ��ǥ �̵� ���ɿ��� Ȯ��
	//--------------------------------------------------------------------
	if (sectorX < dfSECTOR_MAX_X && sectorY < dfSECTOR_MAX_Y)
		return true;

	return false;
}
void ChatServer::AddPlayer_Sector(PLAYER* player, int sectorX, int sectorY)
{
	//--------------------------------------------------------------------
	// ���� ����Ʈ�� �÷��̾� �߰�
	//--------------------------------------------------------------------
	player->sector.x = sectorX;
	player->sector.y = sectorY;
	_sectorList[player->sector.y][player->sector.x].push_back(player);
}
void ChatServer::RemovePlayer_Sector(PLAYER* player)
{
	//--------------------------------------------------------------------
	// ���� ����Ʈ���� �÷��̾� ����
	//--------------------------------------------------------------------
	_sectorList[player->sector.y][player->sector.x].remove(player);
}
void ChatServer::UpdatePlayer_Sector(PLAYER* player, int sectorX, int sectorY)
{
	SRWLock* addSectorLock;
	SRWLock* delSectorLock;

	addSectorLock = &_sectorLockTable[sectorY][sectorX];;
	if (player->sector.x == dfUNKNOWN_SECTOR || player->sector.y == dfUNKNOWN_SECTOR)
	{
		addSectorLock->Lock();
		AddPlayer_Sector(player, sectorX, sectorY);
		addSectorLock->UnLock();
		return;
	}

	delSectorLock = &_sectorLockTable[player->sector.y][player->sector.x];
	while (1)
	{
		delSectorLock->Lock();
		if (addSectorLock->TryLock())
		{
			RemovePlayer_Sector(player);
			AddPlayer_Sector(player, sectorX, sectorY);
			addSectorLock->UnLock();
			delSectorLock->UnLock();
			break;
		}
		delSectorLock->UnLock();
	}
}
void ChatServer::GetSectorAround(int sectorX, int sectorY, SECTOR_AROUND* sectorAround)
{
	//--------------------------------------------------------------------
	// Ư�� ��ǥ �������� �ֺ� ����� ���� ���
	//--------------------------------------------------------------------
	sectorX--;
	sectorY--;

	sectorAround->count = 0;
	for (int y = 0; y < 3; y++)
	{
		if (sectorY + y < 0 || sectorY + y >= dfSECTOR_MAX_Y)
			continue;

		for (int x = 0; x < 3; x++)
		{
			if (sectorX + x < 0 || sectorX + x >= dfSECTOR_MAX_X)
				continue;

			sectorAround->around[sectorAround->count].x = sectorX + x;
			sectorAround->around[sectorAround->count].y = sectorY + y;
			sectorAround->count++;
		}
	}
}
void ChatServer::SendSectorOne(NetPacket* packet, int sectorX, int sectorY)
{
	//--------------------------------------------------------------------
	// Ư�� ���Ϳ� �ִ� �����鿡�� �޽��� ������
	//--------------------------------------------------------------------
	std::list<PLAYER*> *sectorList = &_sectorList[sectorY][sectorX];
	for (auto iter = sectorList->begin(); iter != sectorList->end(); ++iter)
	{
		SendPacket((*iter)->sessionID, packet);
	}
}
void ChatServer::SendSectorAround(PLAYER* player, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �����鿡�� �޽��� ������
	//--------------------------------------------------------------------
	SECTOR_AROUND sectorAround;
	GetSectorAround(player->sector.x, player->sector.y, &sectorAround);

	LockSectorAround(&sectorAround);
	for (int i = 0; i < sectorAround.count; i++)
	{
		SendSectorOne(packet, sectorAround.around[i].x, sectorAround.around[i].y);
	}
	UnLockSectorAround(&sectorAround);
}
void ChatServer::LockSectorAround(SECTOR_AROUND* sectorAround)
{
	SRWLock* sectorLock;
	for (int i = 0; i < sectorAround->count; i++)
	{
		sectorLock = &_sectorLockTable[sectorAround->around[i].y][sectorAround->around[i].x];
		sectorLock->Lock_Shared();
	}
}
void ChatServer::UnLockSectorAround(SECTOR_AROUND* sectorAround)
{
	SRWLock* sectorLock;
	for (int i = 0; i < sectorAround->count; i++)
	{
		sectorLock = &_sectorLockTable[sectorAround->around[i].y][sectorAround->around[i].x];
		sectorLock->UnLock_Shared();
	}
}
bool ChatServer::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// ���� �޽��� Ÿ�Կ� ���� �б� ó��
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		return PacketProc_ChatLogin(sessionID, packet);
	case en_PACKET_CS_CHAT_RES_LOGIN:
		break;
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		return PacketProc_ChatSectorMove(sessionID, packet);
	case en_PACKET_CS_CHAT_RES_SECTOR_MOVE:
		break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		return PacketProc_ChatMessage(sessionID, packet);
	case en_PACKET_CS_CHAT_RES_MESSAGE:
		break;
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		return true;
	default:
		break;
	}
	return false;
}
bool ChatServer::PacketProc_ChatLogin(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// �α��� �޽��� ó��
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	INT64 accountNo;
	WCHAR id[20];
	WCHAR nickname[20];
	char sessionKey[64];

	(*packet) >> accountNo;
	if (packet->GetData((char*)&id, sizeof(id)) != sizeof(id))
		return false;

	if (packet->GetData((char*)&nickname, sizeof(nickname)) != sizeof(nickname))
		return false;

	if (packet->GetData(sessionKey, sizeof(sessionKey)) != sizeof(sessionKey))
		return false;

	//--------------------------------------------------------------------
	// �̹� �α����� �������� ����
	//--------------------------------------------------------------------
	if (user->login)
		return false;

	user->accountNo = accountNo;
	wcscpy_s(user->id, sizeof(id) / 2, id);
	wcscpy_s(user->nickname, sizeof(nickname) / 2, nickname);

	NewPlayer(user->sessionID, user->accountNo);
	user->login = true;

	//--------------------------------------------------------------------
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// �ش� �������� �α��� �Ϸ� �޽��� ������
	//--------------------------------------------------------------------
	Packet::MakeChatLogin(resPacket, true, user->accountNo);
	SendPacket(user->sessionID, resPacket);

	//--------------------------------------------------------------------
	// ������ƮǮ�� ����ȭ ���� �ݳ�
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
bool ChatServer::PacketProc_ChatSectorMove(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ���� �̵� ó��
	//--------------------------------------------------------------------
	INT64 accountNo;
	WORD sectorX;
	WORD sectorY;
	(*packet) >> accountNo >> sectorX >> sectorY;

	//--------------------------------------------------------------------
	// �α������� ���� �������� ����
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	if (!user->login)
		return false;

	//--------------------------------------------------------------------
	// ���� ��ȣ ����
	//--------------------------------------------------------------------
	if (user->accountNo != accountNo)
		return false;

	//--------------------------------------------------------------------
	// ������ �̵� ��û�� ��ǥ�� ������ �̵� ������ ��ǥ���� Ȯ��
	//--------------------------------------------------------------------
	if (!IsMovablePlayer(sectorX, sectorY))
		return false;

	//--------------------------------------------------------------------
	// ���� �̵� ó��
	//--------------------------------------------------------------------
	PLAYER* player = FindPlayer(user->accountNo);
	if (player->sector.x != sectorX || player->sector.y != sectorY)
		UpdatePlayer_Sector(player, sectorX, sectorY);

	//--------------------------------------------------------------------
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// �ش� �������� ���� �̵� �޽��� ������
	//--------------------------------------------------------------------
	Packet::MakeChatSectorMove(resPacket
		, player->accountNo
		, player->sector.x
		, player->sector.y);
	SendPacket(player->sessionID, resPacket);

	//--------------------------------------------------------------------
	// ������ƮǮ�� ����ȭ ���� �ݳ�
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
bool ChatServer::PacketProc_ChatMessage(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ä�� �޽��� ó��
	//--------------------------------------------------------------------
	INT64 accountNo;
	WORD messageLen;
	WCHAR message[256];
	(*packet) >> accountNo >> messageLen;

	if (messageLen > sizeof(message))
		return false;

	if (packet->GetData((char*)message, messageLen) != messageLen)
		return false;

	//--------------------------------------------------------------------
	// �α������� ���� �������� ����
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	if (!user->login)
		return false;

	//--------------------------------------------------------------------
	// ���� ��ȣ ����
	//--------------------------------------------------------------------
	if (user->accountNo != accountNo)
		return false;

	//--------------------------------------------------------------------
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// �ֺ� ����� ������ �����鿡�� ä�� �޽��� ������
	//--------------------------------------------------------------------
	Packet::MakeChatMessage(resPacket
		, user->accountNo
		, user->id
		, user->nickname
		, messageLen
		, message);

	PLAYER* player = FindPlayer(user->accountNo);
	SendSectorAround(player, resPacket);

	//--------------------------------------------------------------------
	// ������ƮǮ�� ����ȭ ���� �ݳ�
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
