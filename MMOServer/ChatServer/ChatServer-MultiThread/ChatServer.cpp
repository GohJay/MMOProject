#include "stdafx.h"
#include "ChatServer.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "../../Common/Logger.h"
#include "../../Common/CrashDump.h"
#include "../../Common/StringUtil.h"
#include "../../Lib/Network/include/Error.h"
#include "../../Lib/Network/include/NetException.h"
#pragma comment(lib, "../../Lib/Network/lib64/Network.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/tacopie.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/cpp_redis.lib")

using namespace Jay;

ChatServer::ChatServer() : _playerPool(0), _loginPlayerCount(0)
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
int ChatServer::GetPlayerCount()
{
	return _playerMap.size();
}
int ChatServer::GetLoginPlayerCount()
{
	return _loginPlayerCount;
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
	NewPlayer(sessionID);
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
	PLAYER* player = FindPlayer(sessionID);
	if (player->login)
		_loginPlayerCount--;

	DeletePlayer(sessionID);
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
	//--------------------------------------------------------------------
	// Redis Connect
	//--------------------------------------------------------------------	
	std::string redisIP;
	UnicodeToString(ServerConfig::GetRedisIP(), redisIP);
	_memorydb.connect(redisIP, ServerConfig::GetRedisPort());

	return true;
}
void ChatServer::Release()
{
	//--------------------------------------------------------------------
	// Redis Disconnect
	//--------------------------------------------------------------------
	_memorydb.disconnect();

	PLAYER* player;
	for (auto iter = _playerMap.begin(); iter != _playerMap.end();)
	{
		player = iter->second;
		_playerPool.Free(player);
		iter = _playerMap.erase(iter);
	}
}
void ChatServer::NewPlayer(DWORD64 sessionID)
{
	PLAYER* player = _playerPool.Alloc();
	player->sessionID = sessionID;
	player->sector.x = dfUNKNOWN_SECTOR;
	player->sector.y = dfUNKNOWN_SECTOR;
	player->login = false;

	_playerLock.Lock();
	_playerMap.insert({ sessionID, player });
	_playerLock.UnLock();
}
void ChatServer::DeletePlayer(DWORD64 sessionID)
{
	PLAYER* player;
	SRWLock* sectorLock;

	_playerLock.Lock();
	auto iter = _playerMap.find(sessionID);
	player = iter->second;
	_playerMap.erase(iter);
	_playerLock.UnLock();

	if (player->sector.x != dfUNKNOWN_SECTOR && player->sector.y != dfUNKNOWN_SECTOR)
	{
		sectorLock = &_sectorLockTable[player->sector.y][player->sector.x];
		sectorLock->Lock();
		RemovePlayer_Sector(player);
		sectorLock->UnLock();
	}
	_playerPool.Free(player);
}
PLAYER* ChatServer::FindPlayer(DWORD64 sessionID)
{
	LockGuard_Shared<SRWLock> lockGuard(&_playerLock);
	PLAYER* player;
	auto iter = _playerMap.find(sessionID);
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
	SRWLock* curSectorLock;
	SRWLock* oldSectorLock;

	curSectorLock = &_sectorLockTable[sectorY][sectorX];;
	if (player->sector.x == dfUNKNOWN_SECTOR || player->sector.y == dfUNKNOWN_SECTOR)
	{
		curSectorLock->Lock();
		AddPlayer_Sector(player, sectorX, sectorY);
		curSectorLock->UnLock();
		return;
	}

	oldSectorLock = &_sectorLockTable[player->sector.y][player->sector.x];
	while (1)
	{
		LockGuard<SRWLock> lockGuard(oldSectorLock);
		if (curSectorLock->TryLock())
		{
			RemovePlayer_Sector(player);
			AddPlayer_Sector(player, sectorX, sectorY);
			curSectorLock->UnLock();
			break;
		}
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
bool ChatServer::Auth(__int64 accountNo, char* sessionKey)
{
	LockGuard<SRWLock> lockGuard(&_memorydbLock);

	//--------------------------------------------------------------------
	// Redis ���� ���� ��ū ��� (Key: AccountNo, Value: SessionKey)
	//--------------------------------------------------------------------
	std::string key = std::to_string(accountNo) + "_chat";
	std::string token = std::string(sessionKey, 64);
	std::future<cpp_redis::reply> future = _memorydb.get(key);
	_memorydb.sync_commit();

	//--------------------------------------------------------------------
	// ���� ��ū ����
	//--------------------------------------------------------------------
	WORD type;
	cpp_redis::reply reply = future.get();
	if (reply.is_string() && reply.as_string().compare(token) == 0)
	{
		//--------------------------------------------------------------------
		// Redis ���� ���� �Ϸ��� ��ū ����
		//--------------------------------------------------------------------
		std::vector<std::string> keys(1);
		keys.emplace_back(std::move(key));
		_memorydb.del(keys);
		_memorydb.sync_commit();

		return true;
	}

	return false;
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
	PLAYER* player = FindPlayer(sessionID);
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
	if (player->login)
		return false;

	//--------------------------------------------------------------------
	// ���� ��ū ����
	//--------------------------------------------------------------------
	bool result;
	if (Auth(accountNo, sessionKey))
	{
		wcscpy_s(player->id, sizeof(id) / 2, id);
		wcscpy_s(player->nickname, sizeof(nickname) / 2, nickname);
		player->accountNo = accountNo;
		player->login = true;
		_loginPlayerCount++;
	
		result = true;
	}
	else
		result = false;

	//--------------------------------------------------------------------
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// �ش� �������� �α��� �Ϸ� �޽��� ������
	//--------------------------------------------------------------------
	Packet::MakeChatLogin(resPacket, result, accountNo);
	SendPacket(player->sessionID, resPacket);

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
	PLAYER* player = FindPlayer(sessionID);
	if (!player->login)
		return false;

	//--------------------------------------------------------------------
	// ���� ��ȣ ����
	//--------------------------------------------------------------------
	if (player->accountNo != accountNo)
		return false;

	//--------------------------------------------------------------------
	// ������ �̵� ��û�� ��ǥ�� ������ �̵� ������ ��ǥ���� Ȯ��
	//--------------------------------------------------------------------
	if (!IsMovablePlayer(sectorX, sectorY))
		return false;

	//--------------------------------------------------------------------
	// ���� �̵� ó��
	//--------------------------------------------------------------------
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
	PLAYER* player = FindPlayer(sessionID);
	if (!player->login)
		return false;

	//--------------------------------------------------------------------
	// ���� ��ȣ ����
	//--------------------------------------------------------------------
	if (player->accountNo != accountNo)
		return false;

	//--------------------------------------------------------------------
	// ������ƮǮ���� ����ȭ ���� �Ҵ�
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// �ֺ� ����� ������ �����鿡�� ä�� �޽��� ������
	//--------------------------------------------------------------------
	Packet::MakeChatMessage(resPacket
		, player->accountNo
		, player->id
		, player->nickname
		, messageLen
		, message);
	SendSectorAround(player, resPacket);

	//--------------------------------------------------------------------
	// ������ƮǮ�� ����ȭ ���� �ݳ�
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
