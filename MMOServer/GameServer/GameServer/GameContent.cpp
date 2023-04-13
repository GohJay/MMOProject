#include "stdafx.h"
#include "GameContent.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "ObjectManager.h"
#include "DBPlayerLogin.h"
#include "DBPlayerLogout.h"
#include "DBPlayerDie.h"
#include "DBPlayerRestart.h"
#include "DBGetCristal.h"
#include "DBRecoverHP.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/MathUtil.h"
#include "../../Common/Logger.h"

using namespace Jay;

GameContent::GameContent(GameServer* server) : _server(server)
{
	_server->AttachContent(this, CONTENT_ID_GAME, FRAME_INTERVAL_GAME);
}
GameContent::~GameContent()
{
}
int GameContent::GetPlayerCount()
{
	return _playerMap.size();
}
int GameContent::GetFPS()
{
	return _oldFPS;
}
int GameContent::GetDBWriteTPS()
{
	return _oldDBWriteTPS;
}
int GameContent::GetDBJobQueueCount()
{
	return _dbJobQ.size();
}
void GameContent::OnStart()
{
	//--------------------------------------------------------------------
	// Initial
	//--------------------------------------------------------------------
	Initial();

	//--------------------------------------------------------------------
	// DB ���� ���ӿ� �ʿ��� ������ ���� ��������
	//--------------------------------------------------------------------
	GetGamedbData();

	//--------------------------------------------------------------------
	// ���� ���������� ���� �Ҵ�
	//--------------------------------------------------------------------
	InitMonsterZone();
}
void GameContent::OnStop()
{
	//--------------------------------------------------------------------
	// ������Ʈ �Ŵ����� ��ϵǾ��ִ� ��� ������Ʈ ����
	//--------------------------------------------------------------------
	ObjectManager::GetInstance()->Cleanup();

	//--------------------------------------------------------------------
	// Release
	//--------------------------------------------------------------------
	Release();
}
void GameContent::OnUpdate()
{
	//--------------------------------------------------------------------
	// ������ ���� ������Ʈ ������Ʈ ó��
	//--------------------------------------------------------------------
	ObjectManager::GetInstance()->Update();

	_curFPS++;
}
void GameContent::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ���� ���� ��Ŷ ó��
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		_server->Disconnect(sessionID);
}
void GameContent::OnClientJoin(DWORD64 sessionID)
{
}
void GameContent::OnClientLeave(DWORD64 sessionID)
{
	auto iter = _playerMap.find(sessionID);
	PlayerObject* player = iter->second;
	_playerMap.erase(iter);

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet, player->GetID());
	SendSectorAround(player, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// Ÿ�� �� ���Ϳ��� �÷��̾� ����
	//--------------------------------------------------------------------
	RemoveTile(player);
	RemoveSector(player);

	//--------------------------------------------------------------------
	// DB �� �÷��̾� �α׾ƿ� ���� ����
	//--------------------------------------------------------------------
	DBPostPlayerLogout(player);

	player->DeleteObject();
}
void GameContent::OnContentEnter(DWORD64 sessionID, WPARAM wParam, LPARAM lParam)
{
	PlayerObject* player = (PlayerObject*)wParam;
	player->GameSetup(this);
	_playerMap.insert({ sessionID, player });

	//--------------------------------------------------------------------
	// DB �� �÷��̾��� �α��� ���� ����
	//--------------------------------------------------------------------
	DBPostPlayerLogin(player);

	//--------------------------------------------------------------------
	// ������Ʈ �Ŵ����� �ش� �÷��̾� ���
	//--------------------------------------------------------------------
	ObjectManager::GetInstance()->InsertObject(player);

	//--------------------------------------------------------------------
	// �÷��̾ �׾��ִ� ��� ����� ó��
	//--------------------------------------------------------------------
	if (player->IsDie())
	{
		player->Restart();
		return;
	}

	//--------------------------------------------------------------------
	// Ÿ�� �� ���Ϳ� �ش� �÷��̾� �߰�
	//--------------------------------------------------------------------
	AddTile(player);
	AddSector(player);

	//--------------------------------------------------------------------
	// 1. �ű� �÷��̾�� - �ű� �÷��̾��� ���� ��Ŷ
	// 2. �ű� �÷��̾�� - ���缽�Ϳ� �����ϴ� ������Ʈ���� ���� ��Ŷ
	// 3. ���缽�Ϳ� �����ϴ� �÷��̾�鿡�� - �ű� �÷��̾��� ���� ��Ŷ
	//--------------------------------------------------------------------

	//--------------------------------------------------------------------
	// 1. �ű� �÷��̾�� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeCreateMyCharacter(packet1
		, player->GetID()
		, player->GetCharacterType()
		, player->GetNickname()
		, player->GetPosX()
		, player->GetPosY()
		, player->GetRotation()
		, player->GetCristal()
		, player->GetHP()
		, player->GetExp()
		, player->GetLevel());
	SendUnicast(player, packet1);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------------------------
	// 2. �ű� �÷��̾�� ���缽�Ϳ� �����ϴ� ������Ʈ���� ���� �� �ൿ ��Ŷ ������
	//--------------------------------------------------------------------------------------
	int sectorX = player->GetCurSectorX();
	int sectorY = player->GetCurSectorY();

	SECTOR_AROUND sectorAround;
	GetSectorAround(sectorX, sectorY, &sectorAround);
	for (int i = 0; i < sectorAround.count; i++)
	{
		std::list<BaseObject*>* sectorList = &_sectorList[sectorAround.around[i].y][sectorAround.around[i].x];
		for (auto iter = sectorList->begin(); iter != sectorList->end(); ++iter)
		{
			BaseObject* object = *iter;
			if (object == player)
				continue;

			switch (object->GetType())
			{
			case PLAYER:
				{
					PlayerObject* existPlayer = static_cast<PlayerObject*>(object);

					NetPacket* packet2 = NetPacket::Alloc();

					Packet::MakeCreateOtherCharacter(packet2
						, existPlayer->GetID()
						, existPlayer->GetCharacterType()
						, existPlayer->GetNickname()
						, existPlayer->GetPosX()
						, existPlayer->GetPosY()
						, existPlayer->GetRotation()
						, existPlayer->GetLevel()
						, FALSE
						, existPlayer->IsSit()
						, existPlayer->IsDie());
					SendUnicast(player, packet2);

					NetPacket::Free(packet2);
				}
				break;
			case MONSTER:
				{
					MonsterObject* existMonster = static_cast<MonsterObject*>(object);

					NetPacket* packet2 = NetPacket::Alloc();

					Packet::MakeCreateMonster(packet2
						, existMonster->GetID()
						, existMonster->GetPosX()
						, existMonster->GetPosY()
						, existMonster->GetRotation()
						, FALSE);
					SendUnicast(player, packet2);

					NetPacket::Free(packet2);
				}
				break;
			case CRISTAL:
				{
					CristalObject* existCristal = static_cast<CristalObject*>(object);

					NetPacket* packet2 = NetPacket::Alloc();

					Packet::MakeCreateCristal(packet2
						, existCristal->GetID()
						, existCristal->GetCristalType()
						, existCristal->GetPosX()
						, existCristal->GetPosY());
					SendUnicast(player, packet2);

					NetPacket::Free(packet2);
				}
				break;
			default:
				break;
			}
		}
	}

	//--------------------------------------------------------------------
	// 3. ���缽�Ϳ� �����ϴ� �÷��̾�鿡�� �ű� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet3 = NetPacket::Alloc();

	Packet::MakeCreateOtherCharacter(packet3
		, player->GetID()
		, player->GetCharacterType()
		, player->GetNickname()
		, player->GetPosX()
		, player->GetPosY()
		, player->GetRotation()
		, player->GetLevel()
		, TRUE
		, player->IsSit()
		, player->IsDie());
	SendSectorAround(player, packet3, false);

	NetPacket::Free(packet3);
}
void GameContent::OnContentExit(DWORD64 sessionID)
{
	auto iter = _playerMap.find(sessionID);
	PlayerObject* player = iter->second;
	_playerMap.erase(iter);

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet, player->GetID());
	SendSectorAround(player, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// Ÿ�� �� ���Ϳ��� �÷��̾� ����
	//--------------------------------------------------------------------
	RemoveTile(player);
	RemoveSector(player);
}
void GameContent::SendUnicast(PlayerObject* player, Jay::NetPacket* packet)
{
	_server->SendPacket(player->GetSessionID(), packet);
}
void GameContent::SendSectorOne(BaseObject* exclusion, NetPacket* packet, int sectorX, int sectorY)
{
	//--------------------------------------------------------------------
	// Ư�� ���Ϳ� �ִ� �÷��̾�鿡�� ��Ŷ ������
	//--------------------------------------------------------------------
	std::list<BaseObject*>* sectorList = &_sectorList[sectorY][sectorX];
	for (auto iter = sectorList->begin(); iter != sectorList->end(); ++iter)
	{
		BaseObject* object = *iter;
		if (object->GetType() != PLAYER)
			continue;

		if (object == exclusion)
			continue;

		SendUnicast((PlayerObject*)object, packet);
	}
}
void GameContent::SendSectorAround(BaseObject* object, NetPacket* packet, bool sendMe)
{
	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� ��Ŷ ������
	//--------------------------------------------------------------------
	BaseObject* exclusion = (sendMe == false) ? object : nullptr;
	SECTOR_AROUND sectorAround;
	int sectorX = object->GetCurSectorX();
	int sectorY = object->GetCurSectorY();

	GetSectorAround(sectorX, sectorY, &sectorAround);
	for (int i = 0; i < sectorAround.count; i++)
	{
		SendSectorOne(exclusion, packet, sectorAround.around[i].x, sectorAround.around[i].y);
	}
}
void GameContent::GetSectorAround(int sectorX, int sectorY, SECTOR_AROUND* sectorAround)
{
	//--------------------------------------------------------------------
	// �ֺ� ����� ���� ���
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
void GameContent::GetUpdateSectorAround(BaseObject* object, SECTOR_AROUND* removeSector, SECTOR_AROUND* addSector)
{
	//--------------------------------------------------------------------
	// ���Ϳ��� ���͸� �̵��Ͽ��� �� 
	// ���� ����ǿ��� ���� ���Ϳ� ���� �߰��� ������ ������ ���ϴ� �Լ�
	//--------------------------------------------------------------------
	SECTOR_AROUND oldSectorAround, curSectorAround;
	GetSectorAround(object->GetOldSectorX(), object->GetOldSectorY(), &oldSectorAround);
	GetSectorAround(object->GetCurSectorX(), object->GetCurSectorY(), &curSectorAround);

	// ��������(oldSector) ���� �� ���缽��(curSector) ���� ���� ������ ã�Ƽ� removeSector�� �ִ´�.
	removeSector->count = 0;
	for (int old = 0; old < oldSectorAround.count; old++)
	{
		bool find = false;
		for (int cur = 0; cur < curSectorAround.count; cur++)
		{
			if (oldSectorAround.around[old].x == curSectorAround.around[cur].x &&
				oldSectorAround.around[old].y == curSectorAround.around[cur].y)
			{
				find = true;
				break;
			}
		}
		if (!find)
		{
			removeSector->around[removeSector->count].x = oldSectorAround.around[old].x;
			removeSector->around[removeSector->count].y = oldSectorAround.around[old].y;
			removeSector->count++;
		}
	}

	// ���缽��(curSector) ���� �� ��������(oldSector) ���� ���� ������ ã�Ƽ� addSector�� �ִ´�.
	addSector->count = 0;
	for (int cur = 0; cur < curSectorAround.count; cur++)
	{
		bool find = false;
		for (int old = 0; old < oldSectorAround.count; old++)
		{
			if (oldSectorAround.around[old].x == curSectorAround.around[cur].x &&
				oldSectorAround.around[old].y == curSectorAround.around[cur].y)
			{
				find = true;
				break;
			}
		}
		if (!find)
		{
			addSector->around[addSector->count].x = curSectorAround.around[cur].x;
			addSector->around[addSector->count].y = curSectorAround.around[cur].y;
			addSector->count++;
		}
	}
}
void GameContent::UpdateSectorAround_Player(PlayerObject* player)
{
	//--------------------------------------------------------------------------------------
	// 1. �������Ϳ� �����ϴ� �÷��̾�鿡�� - �̵��ϴ� �÷��̾��� ���� ��Ŷ
	// 2. �̵��ϴ� �÷��̾�� - �������Ϳ� �����ϴ� ������Ʈ�� ���� ��Ŷ
	// 3. ���缽�Ϳ� �����ϴ� �÷��̾�鿡�� - �̵��ϴ� �÷��̾��� ���� ��Ŷ
	// 4. ���缽�Ϳ� �����ϴ� �÷��̾�鿡�� - �̵��ϴ� �÷��̾��� �̵� ��Ŷ
	// 5. �̵��ϴ� �÷��̾�� - ���缽�Ϳ� �����ϴ� ������Ʈ���� ���� ��Ŷ
	//--------------------------------------------------------------------------------------
	SECTOR_AROUND removeSector, addSector;
	GetUpdateSectorAround(player, &removeSector, &addSector);

	//--------------------------------------------------------------------------------------
	// 1. removeSector�� �̵��ϴ� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet1, player->GetID());
	for (int i = 0; i < removeSector.count; i++)
	{
		SendSectorOne(player, packet1, removeSector.around[i].x, removeSector.around[i].y);
	}

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------------------------
	// 2. �̵��ϴ� �÷��̾�� removeSector ������Ʈ���� ���� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	for (int i = 0; i < removeSector.count; i++)
	{
		std::list<BaseObject*>* sectorList = &_sectorList[removeSector.around[i].y][removeSector.around[i].x];
		for (auto iter = sectorList->begin(); iter != sectorList->end(); ++iter)
		{
			NetPacket* packet2 = NetPacket::Alloc();

			Packet::MakeDeleteObject(packet2, (*iter)->GetID());
			SendUnicast(player, packet2);

			NetPacket::Free(packet2);
		}
	}

	//--------------------------------------------------------------------------------------
	// 3. addSector�� �̵��ϴ� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	NetPacket* packet3 = NetPacket::Alloc();

	Packet::MakeCreateOtherCharacter(packet3
		, player->GetID()
		, player->GetCharacterType()
		, player->GetNickname()
		, player->GetPosX()
		, player->GetPosY()
		, player->GetRotation()
		, player->GetLevel()
		, FALSE
		, player->IsSit()
		, player->IsDie());
	for (int i = 0; i < addSector.count; i++)
	{
		SendSectorOne(player, packet3, addSector.around[i].x, addSector.around[i].y);
	}

	NetPacket::Free(packet3);

	//--------------------------------------------------------------------------------------
	// 4. addSector�� �̵��ϴ� �÷��̾��� �̵� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	NetPacket* packet4 = NetPacket::Alloc();

	Packet::MakeMoveCharacterStart(packet4
		, player->GetID()
		, player->GetPosX()
		, player->GetPosY()
		, player->GetRotation()
		, player->GetVKey()
		, player->GetHKey());
	for (int i = 0; i < addSector.count; i++)
	{
		SendSectorOne(player, packet4, addSector.around[i].x, addSector.around[i].y);
	}

	NetPacket::Free(packet4);

	//--------------------------------------------------------------------------------------
	// 5. �̵��ϴ� �÷��̾�� addSector ������Ʈ���� ���� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	for (int i = 0; i < addSector.count; i++)
	{
		std::list<BaseObject*>* sectorList = &_sectorList[addSector.around[i].y][addSector.around[i].x];
		for (auto iter = sectorList->begin(); iter != sectorList->end(); ++iter)
		{
			BaseObject* object = *iter;
			if (object == player)
				continue;

			switch (object->GetType())
			{
			case PLAYER:
				{
					PlayerObject* existPlayer = static_cast<PlayerObject*>(object);

					NetPacket* packet5 = NetPacket::Alloc();

					Packet::MakeCreateOtherCharacter(packet5
						, existPlayer->GetID()
						, existPlayer->GetCharacterType()
						, existPlayer->GetNickname()
						, existPlayer->GetPosX()
						, existPlayer->GetPosY()
						, existPlayer->GetRotation()
						, existPlayer->GetLevel()
						, FALSE
						, existPlayer->IsSit()
						, existPlayer->IsDie());
					SendUnicast(player, packet5);

					NetPacket::Free(packet5);
				}
				break;
			case MONSTER:
				{
					MonsterObject* existMonster = static_cast<MonsterObject*>(object);

					NetPacket* packet5 = NetPacket::Alloc();

					Packet::MakeCreateMonster(packet5
						, existMonster->GetID()
						, existMonster->GetPosX()
						, existMonster->GetPosY()
						, existMonster->GetRotation()
						, FALSE);
					SendUnicast(player, packet5);

					NetPacket::Free(packet5);
				}
				break;
			case CRISTAL:
				{
					CristalObject* existCristal = static_cast<CristalObject*>(object);

					NetPacket* packet5 = NetPacket::Alloc();

					Packet::MakeCreateCristal(packet5
						, existCristal->GetID()
						, existCristal->GetCristalType()
						, existCristal->GetPosX()
						, existCristal->GetPosY());
					SendUnicast(player, packet5);

					NetPacket::Free(packet5);
				}
				break;
			default:
				break;
			}
		}
	}
}
void GameContent::UpdateSectorAround_Monster(MonsterObject* monster)
{
	//--------------------------------------------------------------------------------------
	// 1. �������Ϳ� �����ϴ� �÷��̾�鿡�� - �̵��ϴ� ������Ʈ�� ���� ��Ŷ
	// 2. ���缽�Ϳ� �����ϴ� �÷��̾�鿡�� - �̵��ϴ� ������Ʈ�� ���� ��Ŷ
	//--------------------------------------------------------------------------------------
	SECTOR_AROUND removeSector, addSector;
	GetUpdateSectorAround(monster, &removeSector, &addSector);

	//--------------------------------------------------------------------------------------
	// 1. removeSector�� �̵��ϴ� ������ ���� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet1, monster->GetID());
	for (int i = 0; i < removeSector.count; i++)
	{
		SendSectorOne(nullptr, packet1, removeSector.around[i].x, removeSector.around[i].y);
	}

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------------------------
	// 2. addSector�� �̵��ϴ� ������ ���� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	NetPacket* packet2 = NetPacket::Alloc();

	Packet::MakeCreateMonster(packet2
		, monster->GetID()
		, monster->GetPosX()
		, monster->GetPosY()
		, monster->GetRotation()
		, FALSE);
	for (int i = 0; i < addSector.count; i++)
	{
		SendSectorOne(nullptr, packet2, addSector.around[i].x, addSector.around[i].y);
	}

	NetPacket::Free(packet2);
}
void GameContent::AddSector(BaseObject* object)
{
	//--------------------------------------------------------------------
	// ������Ʈ�� ���� ��ǥ�� ���� ��ġ�� ��� �ش� ���Ϳ� �߰�
	//--------------------------------------------------------------------
	int sectorX = object->GetCurSectorX();
	int sectorY = object->GetCurSectorY();
	_sectorList[sectorY][sectorX].push_back(object);
}
void GameContent::RemoveSector(BaseObject* object)
{
	//--------------------------------------------------------------------
	// ������Ʈ�� ���� ��ǥ�� ���� ��ġ�� ��� �ش� ���Ϳ��� ����
	//--------------------------------------------------------------------
	int sectorX = object->GetCurSectorX();
	int sectorY = object->GetCurSectorY();
	_sectorList[sectorY][sectorX].remove(object);
}
void GameContent::AddTile(BaseObject* object)
{
	//--------------------------------------------------------------------
	// ������Ʈ�� ���� ��ǥ�� Ÿ�� ��ġ�� ����Ͽ� �ش� Ÿ�Ͽ� �߰�
	//--------------------------------------------------------------------
	int tileX = object->GetTileX();
	int tileY = object->GetTileY();
	_tileList[tileY][tileX].push_back(object);
}
void GameContent::RemoveTile(BaseObject* object)
{
	//--------------------------------------------------------------------
	// ������Ʈ�� ���� ��ǥ�� Ÿ�� ��ġ�� ����Ͽ� �ش� Ÿ�Ͽ��� ����
	//--------------------------------------------------------------------
	int tileX = object->GetTileX();
	int tileY = object->GetTileY();
	_tileList[tileY][tileX].remove(object);
}
bool GameContent::IsTileOut(int tileX, int tileY)
{
	if (tileX < 0 || tileX >= dfMAP_TILE_MAX_X || tileY < 0 || tileY >= dfMAP_TILE_MAX_Y)
		return true;

	return false;
}
std::list<BaseObject*>* GameContent::GetTile(int tileX, int tileY)
{
	return &_tileList[tileY][tileX];
}
std::list<BaseObject*>* GameContent::GetSector(int sectorX, int sectorY)
{
	return &_sectorList[sectorY][sectorX];
}
DATA_CRISTAL* GameContent::GetDataCristal(INT64 type)
{
	return _dataCristalMap[type];
}
DATA_MONSTER* GameContent::GetDataMonster(INT64 type)
{
	return _dataMonsterMap[type];
}
DATA_PLAYER* GameContent::GetDataPlayer()
{
	return &_dataPlayer;
}
void GameContent::CreateCristal(float posX, float posY)
{
	CristalObject* cristal = CristalObject::Alloc(this);
	BYTE cristalType = (rand() % _dataCristalMap.size()) + 1;
	cristal->Init(cristalType, posX, posY);

	//--------------------------------------------------------------------------------------
	// ������Ʈ �Ŵ����� �ش� ũ����Ż �߰�
	//--------------------------------------------------------------------------------------
	ObjectManager::GetInstance()->InsertObject(cristal);

	//--------------------------------------------------------------------------------------
	// Ÿ�� �� ���Ϳ� �ش� ũ����Ż �߰�
	//--------------------------------------------------------------------------------------
	AddTile(cristal);
	AddSector(cristal);

	//--------------------------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �����ϴ� �÷��̾�鿡�� �ű� ũ����Ż�� ���� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeCreateCristal(packet
		, cristal->GetID()
		, cristal->GetCristalType()
		, cristal->GetPosX()
		, cristal->GetPosY());
	SendSectorAround(cristal, packet, false);

	NetPacket::Free(packet);
}
void GameContent::DeleteCristal(CristalObject* cristal)
{
	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� ũ����Ż�� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet, cristal->GetID());
	SendSectorAround(cristal, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// Ÿ�� �� ���Ϳ��� ũ����Ż ����
	//--------------------------------------------------------------------
	RemoveTile(cristal);
	RemoveSector(cristal);

	cristal->DeleteObject();
}
void GameContent::DBPostPlayerLogin(PlayerObject* player)
{
	DBPlayerLogin* dbJob = new DBPlayerLogin(&_gamedb
		, player->GetAccountNo()
		, player->GetTileX()
		, player->GetTileY()
		, player->GetCristal()
		, player->GetHP()
	);

	_dbJobQ.Enqueue(dbJob);
	SetEvent(_hDBJobEvent);
}
void GameContent::DBPostPlayerLogout(PlayerObject* player)
{
	DBPlayerLogout* dbJob = new DBPlayerLogout(&_gamedb
		, player->GetAccountNo()
		, player->GetPosX()
		, player->GetPosY()
		, player->GetTileX()
		, player->GetTileY()
		, player->GetRotation()
		, player->GetHP()
		, player->GetExp()
	);

	_dbJobQ.Enqueue(dbJob);
	SetEvent(_hDBJobEvent);
}
void GameContent::DBPostPlayerDie(PlayerObject* player, int minusCristal)
{
	DBPlayerDie* dbJob = new DBPlayerDie(&_gamedb
		, player->GetAccountNo()
		, player->GetTileX()
		, player->GetTileY()
		, minusCristal
	);

	_dbJobQ.Enqueue(dbJob);
	SetEvent(_hDBJobEvent);
}
void GameContent::DBPostPlayerRestart(PlayerObject* player)
{
	DBPlayerRestart* dbJob = new DBPlayerRestart(&_gamedb
		, player->GetAccountNo()
		, player->GetTileX()
		, player->GetTileY()
	);

	_dbJobQ.Enqueue(dbJob);
	SetEvent(_hDBJobEvent);
}
void GameContent::DBPostGetCristal(PlayerObject* player, int getCristal)
{
	DBGetCristal* dbJob = new DBGetCristal(&_gamedb
		, player->GetAccountNo()
		, getCristal
	);

	_dbJobQ.Enqueue(dbJob);
	SetEvent(_hDBJobEvent);
}
void GameContent::DBPostRecoverHP(PlayerObject* player, int oldHP, int sitTimeSec)
{
	DBRecoverHP* dbJob = new DBRecoverHP(&_gamedb
		, player->GetAccountNo()
		, oldHP
		, player->GetHP()
		, sitTimeSec
	);

	_dbJobQ.Enqueue(dbJob);
	SetEvent(_hDBJobEvent);
}
bool GameContent::Initial()
{
	_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_hDBJobEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	//--------------------------------------------------------------------
	// Database Connect
	//--------------------------------------------------------------------
	_gamedb.Connect(ServerConfig::GetDatabaseIP()
		, ServerConfig::GetDatabasePort()
		, ServerConfig::GetDatabaseUser()
		, ServerConfig::GetDatabasePassword()
		, ServerConfig::GetDatabaseSchema());

	//--------------------------------------------------------------------
	// Thread Begin
	//--------------------------------------------------------------------
	_dbWriteThread = std::thread(&GameContent::DBWriteThread, this);
	_managementThread = std::thread(&GameContent::ManagementThread, this);

	return true;
}
void GameContent::Release()
{
	//--------------------------------------------------------------------
	// Thread End
	//--------------------------------------------------------------------
	SetEvent(_hExitEvent);
	_dbWriteThread.join();
	_managementThread.join();

	//--------------------------------------------------------------------
	// Database Disconnect
	//--------------------------------------------------------------------
	_gamedb.Disconnect();

	CloseHandle(_hExitEvent);
	CloseHandle(_hDBJobEvent);
}
void GameContent::DBWriteThread()
{
	HANDLE hHandle[2] = { _hDBJobEvent, _hExitEvent };
	DWORD ret;
	while (1)
	{
		ret = WaitForMultipleObjects(2, hHandle, FALSE, INFINITE);
		if ((ret - WAIT_OBJECT_0) != 0)
			break;

		IDBJob* job;
		_dbJobQ.Flip();
		while (_dbJobQ.size() > 0)
		{
			_dbJobQ.Dequeue(job);
			job->Exec();
			delete job;
			_curDBWriteTPS++;
		}
	}
}
void GameContent::ManagementThread()
{
	HANDLE hHandle[2] = { _hDBJobEvent, _hExitEvent };
	DWORD ret;
	while (1)
	{
		ret = WaitForSingleObject(_hExitEvent, 1000);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			return;
		case WAIT_TIMEOUT:
			UpdateFPS();
			UpdateDBWriteTPS();
			break;
		default:
			break;
		}
	}
}
void GameContent::UpdateFPS()
{
	_oldFPS.exchange(_curFPS.exchange(0));
}
void GameContent::UpdateDBWriteTPS()
{
	_oldDBWriteTPS.exchange(_curDBWriteTPS.exchange(0));
}
void GameContent::GetGamedbData()
{
	//--------------------------------------------------------------------
	// DB ���� ũ����Ż ������ ���� ��������
	//--------------------------------------------------------------------
	sql::ResultSet* res;
	res = _gamedb.ExecuteQuery(L"SELECT cristal_type, amount FROM gamedb.data_cristal ORDER BY cristal_type ASC;");
	while (res->next())
	{
		DATA_CRISTAL* dataCristal = new DATA_CRISTAL;
		dataCristal->type = res->getInt64(1);
		dataCristal->amount = res->getInt(2);
		_dataCristalMap.insert({ dataCristal->type, dataCristal });
	}
	_gamedb.ClearQuery(res);

	//--------------------------------------------------------------------
	// DB ���� ���� ������ ���� ��������
	//--------------------------------------------------------------------
	res = _gamedb.ExecuteQuery(L"SELECT monster_type, hp, damage FROM gamedb.data_monster ORDER BY monster_type ASC;");
	while (res->next())
	{
		DATA_MONSTER* dataMonster = new DATA_MONSTER;
		dataMonster->type = res->getInt64(1);
		dataMonster->hp = res->getInt(2);
		dataMonster->damage = res->getInt(3);
		_dataMonsterMap.insert({ dataMonster->type, dataMonster });
	}
	_gamedb.ClearQuery(res);

	//--------------------------------------------------------------------
	// DB ���� �÷��̾� ������ ���� ��������
	//--------------------------------------------------------------------
	res = _gamedb.ExecuteQuery(L"SELECT damage, hp, recovery_hp FROM gamedb.data_player;");
	if (res->next())
	{
		_dataPlayer.damage = res->getInt(1);
		_dataPlayer.hp = res->getInt(2);
		_dataPlayer.recovery_hp = res->getInt(3);
	}
	_gamedb.ClearQuery(res);
}
void GameContent::InitMonsterZone()
{
	//--------------------------------------------------------------------
	// ���� ������ ������ ��å��� ���͸� �Ҵ��ϱ�
	//--------------------------------------------------------------------
	MonsterObject* monster;

	for (int i = 0; i < dfMONSTER_RESPAWN_COUNT_ZONE_1; i++)
	{
		BYTE monsterType = (rand() % _dataMonsterMap.size()) + 1;
		monster = MonsterObject::Alloc(this);
		monster->Init(monsterType, ZONE_1);
		ObjectManager::GetInstance()->InsertObject(monster);
	}

	for (int i = 0; i < dfMONSTER_RESPAWN_COUNT_ZONE_2; i++)
	{
		BYTE monsterType = (rand() % _dataMonsterMap.size()) + 1;
		monster = MonsterObject::Alloc(this);
		monster->Init(monsterType, ZONE_2);
		ObjectManager::GetInstance()->InsertObject(monster);
	}

	for (int i = 0; i < dfMONSTER_RESPAWN_COUNT_ZONE_3; i++)
	{
		BYTE monsterType = (rand() % _dataMonsterMap.size()) + 1;
		monster = MonsterObject::Alloc(this);
		monster->Init(monsterType, ZONE_3);
		ObjectManager::GetInstance()->InsertObject(monster);
	}

	for (int i = 0; i < dfMONSTER_RESPAWN_COUNT_ZONE_4; i++)
	{
		BYTE monsterType = (rand() % _dataMonsterMap.size()) + 1;
		monster = MonsterObject::Alloc(this);
		monster->Init(monsterType, ZONE_4);
		ObjectManager::GetInstance()->InsertObject(monster);
	}

	for (int i = 0; i < dfMONSTER_RESPAWN_COUNT_ZONE_5; i++)
	{
		BYTE monsterType = (rand() % _dataMonsterMap.size()) + 1;
		monster = MonsterObject::Alloc(this);
		monster->Init(monsterType, ZONE_5);
		ObjectManager::GetInstance()->InsertObject(monster);
	}

	for (int i = 0; i < dfMONSTER_RESPAWN_COUNT_ZONE_6; i++)
	{
		BYTE monsterType = (rand() % _dataMonsterMap.size()) + 1;
		monster = MonsterObject::Alloc(this);
		monster->Init(monsterType, ZONE_6);
		ObjectManager::GetInstance()->InsertObject(monster);
	}

	for (int i = 0; i < dfMONSTER_RESPAWN_COUNT_ZONE_7; i++)
	{
		BYTE monsterType = (rand() % _dataMonsterMap.size()) + 1;
		monster = MonsterObject::Alloc(this);
		monster->Init(monsterType, ZONE_7);
		ObjectManager::GetInstance()->InsertObject(monster);
	}
}
bool GameContent::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// ���� ��Ŷ Ÿ�Կ� ���� �б� ó��
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_CS_GAME_REQ_MOVE_CHARACTER:
		return PacketProc_MoveStart(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_STOP_CHARACTER:
		return PacketProc_MoveStop(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_ATTACK1:
		return PacketProc_Attak1(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_ATTACK2:
		return PacketProc_Attak2(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_PICK:
		return PacketProc_Pick(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_SIT:
		return PacketProc_Sit(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_PLAYER_RESTART:
		return PacketProc_Restart(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_ECHO:
		return PacketProc_GameEcho(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		return true;
	default:
		break;
	}
	return false;
}
bool GameContent::PacketProc_MoveStart(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	float posX;
	float posY;
	USHORT rotation;
	BYTE vKey;
	BYTE hKey;
	(*packet) >> clientID;
	(*packet) >> posX;
	(*packet) >> posY;
	(*packet) >> rotation;
	(*packet) >> vKey;
	(*packet) >> hKey;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (player->GetID() != clientID)
	{
		Logger::WriteLog(L"Game"
			, LOG_LEVEL_DEBUG
			, L"func: %s, line: %d, error: Mismatch ID, objectID: %lld, clientID: %lld"
			, __FUNCTIONW__
			, __LINE__
			, player->GetID()
			, clientID);
		return false;
	}

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// �÷��̾� �̵� ó��
	//--------------------------------------------------------------------
	player->MoveStart(posX, posY, rotation, vKey, hKey);

	return true;
}
bool GameContent::PacketProc_MoveStop(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	float posX;
	float posY;
	USHORT rotation;
	(*packet) >> clientID;
	(*packet) >> posX;
	(*packet) >> posY;
	(*packet) >> rotation;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (player->GetID() != clientID)
	{
		Logger::WriteLog(L"Game"
			, LOG_LEVEL_DEBUG
			, L"func: %s, line: %d, error: Mismatch ID, objectID: %lld, clientID: %lld"
			, __FUNCTIONW__
			, __LINE__
			, player->GetID()
			, clientID);
		return false;
	}

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ó��
	//--------------------------------------------------------------------
	player->MoveStop(posX, posY, rotation);

	return true;
}
bool GameContent::PacketProc_Attak1(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	(*packet) >> clientID;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (player->GetID() != clientID)
	{
		Logger::WriteLog(L"Game"
			, LOG_LEVEL_DEBUG
			, L"func: %s, line: %d, error: Mismatch ID, objectID: %lld, clientID: %lld"
			, __FUNCTIONW__
			, __LINE__
			, player->GetID()
			, clientID);
		return false;
	}

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ó��
	//--------------------------------------------------------------------
	player->Attack1();

	return true;
}
bool GameContent::PacketProc_Attak2(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	(*packet) >> clientID;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (player->GetID() != clientID)
	{
		Logger::WriteLog(L"Game"
			, LOG_LEVEL_DEBUG
			, L"func: %s, line: %d, error: Mismatch ID, objectID: %lld, clientID: %lld"
			, __FUNCTIONW__
			, __LINE__
			, player->GetID()
			, clientID);
		return false;
	}

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ó��
	//--------------------------------------------------------------------
	player->Attack2();

	return true;
}
bool GameContent::PacketProc_Pick(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	(*packet) >> clientID;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (player->GetID() != clientID)
	{
		Logger::WriteLog(L"Game"
			, LOG_LEVEL_DEBUG
			, L"func: %s, line: %d, error: Mismatch ID, objectID: %lld, clientID: %lld"
			, __FUNCTIONW__
			, __LINE__
			, player->GetID()
			, clientID);
		return false;
	}

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// �÷��̾� �ݱ� ���� ó��
	//--------------------------------------------------------------------
	player->Pick();

	return true;
}
bool GameContent::PacketProc_Sit(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	(*packet) >> clientID;

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (player->GetID() != clientID)
	{
		Logger::WriteLog(L"Game"
			, LOG_LEVEL_DEBUG
			, L"func: %s, line: %d, error: Mismatch ID, objectID: %lld, clientID: %lld"
			, __FUNCTIONW__
			, __LINE__
			, player->GetID()
			, clientID);
		return false;
	}

	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// �÷��̾� ������
	//--------------------------------------------------------------------
	player->Sit();

	return true;
}
bool GameContent::PacketProc_Restart(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// �÷��̾� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (!player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet1, player->GetID());
	SendSectorAround(player, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// ���� ���Ϳ��� �÷��̾� ����
	//--------------------------------------------------------------------
	RemoveSector(player);

	//--------------------------------------------------------------------
	// �÷��̾� ����� ó��
	//--------------------------------------------------------------------
	player->Restart();

	return true;
}
bool GameContent::PacketProc_GameEcho(DWORD64 sessionID, NetPacket* packet)
{
	INT64 accountNo;
	LONGLONG sendTick;
	(*packet) >> accountNo;
	(*packet) >> sendTick;

	//--------------------------------------------------------------------
	// �ش� �������� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	Packet::MakeGameEcho(resPacket, accountNo, sendTick);
	PlayerObject* player = _playerMap[sessionID];
	SendUnicast(player, resPacket);

	NetPacket::Free(resPacket);
	return true;
}
