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
	// DB 에서 게임에 필요한 데이터 정보 가져오기
	//--------------------------------------------------------------------
	GetGamedbData();

	//--------------------------------------------------------------------
	// 몬스터 출현지역에 몬스터 할당
	//--------------------------------------------------------------------
	InitMonsterZone();
}
void GameContent::OnStop()
{
	//--------------------------------------------------------------------
	// 오브젝트 매니저에 등록되어있는 모든 오브젝트 정리
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
	// 프레임 단위 오브젝트 업데이트 처리
	//--------------------------------------------------------------------
	ObjectManager::GetInstance()->Update();

	_curFPS++;
}
void GameContent::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 수신 받은 패킷 처리
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
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 제거 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet, player->GetID());
	SendSectorAround(player, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// 타일 및 섹터에서 플레이어 제거
	//--------------------------------------------------------------------
	RemoveTile(player);
	RemoveSector(player);

	//--------------------------------------------------------------------
	// DB 에 플레이어 로그아웃 정보 저장
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
	// DB 에 플레이어의 로그인 정보 저장
	//--------------------------------------------------------------------
	DBPostPlayerLogin(player);

	//--------------------------------------------------------------------
	// 오브젝트 매니저에 해당 플레이어 등록
	//--------------------------------------------------------------------
	ObjectManager::GetInstance()->InsertObject(player);

	//--------------------------------------------------------------------
	// 플레이어가 죽어있던 경우 재시작 처리
	//--------------------------------------------------------------------
	if (player->IsDie())
	{
		player->Restart();
		return;
	}

	//--------------------------------------------------------------------
	// 타일 및 섹터에 해당 플레이어 추가
	//--------------------------------------------------------------------
	AddTile(player);
	AddSector(player);

	//--------------------------------------------------------------------
	// 1. 신규 플레이어에게 - 신규 플레이어의 생성 패킷
	// 2. 신규 플레이어에게 - 현재섹터에 존재하는 오브젝트들의 생성 패킷
	// 3. 현재섹터에 존재하는 플레이어들에게 - 신규 플레이어의 생성 패킷
	//--------------------------------------------------------------------

	//--------------------------------------------------------------------
	// 1. 신규 플레이어에게 생성 패킷 보내기
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
	// 2. 신규 플레이어에게 현재섹터에 존재하는 오브젝트들의 생성 및 행동 패킷 보내기
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
	// 3. 현재섹터에 존재하는 플레이어들에게 신규 플레이어의 생성 패킷 보내기
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
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 제거 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet, player->GetID());
	SendSectorAround(player, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// 타일 및 섹터에서 플레이어 제거
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
	// 특정 섹터에 있는 플레이어들에게 패킷 보내기
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
	// 주변 영향권 섹터에 있는 플레이어들에게 패킷 보내기
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
	// 주변 영향권 섹터 얻기
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
	// 섹터에서 섹터를 이동하였을 때 
	// 섹터 영향권에서 빠진 섹터와 새로 추가된 섹터의 정보를 구하는 함수
	//--------------------------------------------------------------------
	SECTOR_AROUND oldSectorAround, curSectorAround;
	GetSectorAround(object->GetOldSectorX(), object->GetOldSectorY(), &oldSectorAround);
	GetSectorAround(object->GetCurSectorX(), object->GetCurSectorY(), &curSectorAround);

	// 이전섹터(oldSector) 정보 중 현재섹터(curSector) 에는 없는 정보를 찾아서 removeSector에 넣는다.
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

	// 현재섹터(curSector) 정보 중 이전섹터(oldSector) 에는 없는 정보를 찾아서 addSector에 넣는다.
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
	// 1. 이전섹터에 존재하는 플레이어들에게 - 이동하는 플레이어의 삭제 패킷
	// 2. 이동하는 플레이어에게 - 이전섹터에 존재하는 오브젝트의 삭제 패킷
	// 3. 현재섹터에 존재하는 플레이어들에게 - 이동하는 플레이어의 생성 패킷
	// 4. 현재섹터에 존재하는 플레이어들에게 - 이동하는 플레이어의 이동 패킷
	// 5. 이동하는 플레이어에게 - 현재섹터에 존재하는 오브젝트들의 생성 패킷
	//--------------------------------------------------------------------------------------
	SECTOR_AROUND removeSector, addSector;
	GetUpdateSectorAround(player, &removeSector, &addSector);

	//--------------------------------------------------------------------------------------
	// 1. removeSector에 이동하는 플레이어의 삭제 패킷 보내기
	//--------------------------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet1, player->GetID());
	for (int i = 0; i < removeSector.count; i++)
	{
		SendSectorOne(player, packet1, removeSector.around[i].x, removeSector.around[i].y);
	}

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------------------------
	// 2. 이동하는 플레이어에게 removeSector 오브젝트들의 삭제 패킷 보내기
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
	// 3. addSector에 이동하는 플레이어의 생성 패킷 보내기
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
	// 4. addSector에 이동하는 플레이어의 이동 패킷 보내기
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
	// 5. 이동하는 플레이어에게 addSector 오브젝트들의 생성 패킷 보내기
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
	// 1. 이전섹터에 존재하는 플레이어들에게 - 이동하는 오브젝트의 삭제 패킷
	// 2. 현재섹터에 존재하는 플레이어들에게 - 이동하는 오브젝트의 생성 패킷
	//--------------------------------------------------------------------------------------
	SECTOR_AROUND removeSector, addSector;
	GetUpdateSectorAround(monster, &removeSector, &addSector);

	//--------------------------------------------------------------------------------------
	// 1. removeSector에 이동하는 몬스터의 삭제 패킷 보내기
	//--------------------------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet1, monster->GetID());
	for (int i = 0; i < removeSector.count; i++)
	{
		SendSectorOne(nullptr, packet1, removeSector.around[i].x, removeSector.around[i].y);
	}

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------------------------
	// 2. addSector에 이동하는 몬스터의 생성 패킷 보내기
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
	// 오브젝트의 현재 좌표로 섹터 위치를 얻어 해당 섹터에 추가
	//--------------------------------------------------------------------
	int sectorX = object->GetCurSectorX();
	int sectorY = object->GetCurSectorY();
	_sectorList[sectorY][sectorX].push_back(object);
}
void GameContent::RemoveSector(BaseObject* object)
{
	//--------------------------------------------------------------------
	// 오브젝트의 현재 좌표로 섹터 위치를 얻어 해당 섹터에서 삭제
	//--------------------------------------------------------------------
	int sectorX = object->GetCurSectorX();
	int sectorY = object->GetCurSectorY();
	_sectorList[sectorY][sectorX].remove(object);
}
void GameContent::AddTile(BaseObject* object)
{
	//--------------------------------------------------------------------
	// 오브젝트의 현재 좌표로 타일 위치를 계산하여 해당 타일에 추가
	//--------------------------------------------------------------------
	int tileX = object->GetTileX();
	int tileY = object->GetTileY();
	_tileList[tileY][tileX].push_back(object);
}
void GameContent::RemoveTile(BaseObject* object)
{
	//--------------------------------------------------------------------
	// 오브젝트의 현재 좌표로 타일 위치를 계산하여 해당 타일에서 삭제
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
	// 오브젝트 매니저에 해당 크리스탈 추가
	//--------------------------------------------------------------------------------------
	ObjectManager::GetInstance()->InsertObject(cristal);

	//--------------------------------------------------------------------------------------
	// 타일 및 섹터에 해당 크리스탈 추가
	//--------------------------------------------------------------------------------------
	AddTile(cristal);
	AddSector(cristal);

	//--------------------------------------------------------------------------------------
	// 주변 영향권 섹터에 존재하는 플레이어들에게 신규 크리스탈의 생성 패킷 보내기
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
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 크리스탈의 제거 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet, cristal->GetID());
	SendSectorAround(cristal, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// 타일 및 섹터에서 크리스탈 제거
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
	// DB 에서 크리스탈 데이터 정보 가져오기
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
	// DB 에서 몬스터 데이터 정보 가져오기
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
	// DB 에서 플레이어 데이터 정보 가져오기
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
	// 몬스터 리스폰 구역별 정책대로 몬스터를 할당하기
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
	// 수신 패킷 타입에 따른 분기 처리
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
	// 플레이어 본인 여부 확인
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
	// 플레이어 생존 여부 확인
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// 플레이어 이동 처리
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
	// 플레이어 본인 여부 확인
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
	// 플레이어 생존 여부 확인
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// 플레이어 정지 처리
	//--------------------------------------------------------------------
	player->MoveStop(posX, posY, rotation);

	return true;
}
bool GameContent::PacketProc_Attak1(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	(*packet) >> clientID;

	//--------------------------------------------------------------------
	// 플레이어 본인 여부 확인
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
	// 플레이어 생존 여부 확인
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// 플레이어 공격 처리
	//--------------------------------------------------------------------
	player->Attack1();

	return true;
}
bool GameContent::PacketProc_Attak2(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	(*packet) >> clientID;

	//--------------------------------------------------------------------
	// 플레이어 본인 여부 확인
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
	// 플레이어 생존 여부 확인
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// 플레이어 공격 처리
	//--------------------------------------------------------------------
	player->Attack2();

	return true;
}
bool GameContent::PacketProc_Pick(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	(*packet) >> clientID;

	//--------------------------------------------------------------------
	// 플레이어 본인 여부 확인
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
	// 플레이어 생존 여부 확인
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// 플레이어 줍기 동작 처리
	//--------------------------------------------------------------------
	player->Pick();

	return true;
}
bool GameContent::PacketProc_Sit(DWORD64 sessionID, NetPacket* packet)
{
	INT64 clientID;
	(*packet) >> clientID;

	//--------------------------------------------------------------------
	// 플레이어 본인 여부 확인
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
	// 플레이어 생존 여부 확인
	//--------------------------------------------------------------------
	if (player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// 플레이어 앉히기
	//--------------------------------------------------------------------
	player->Sit();

	return true;
}
bool GameContent::PacketProc_Restart(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 플레이어 생존 여부 확인
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (!player->IsDie())
		return true;

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 삭제 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet1, player->GetID());
	SendSectorAround(player, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// 기존 섹터에서 플레이어 제거
	//--------------------------------------------------------------------
	RemoveSector(player);

	//--------------------------------------------------------------------
	// 플레이어 재시작 처리
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
	// 해당 유저에게 에코 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	Packet::MakeGameEcho(resPacket, accountNo, sendTick);
	PlayerObject* player = _playerMap[sessionID];
	SendUnicast(player, resPacket);

	NetPacket::Free(resPacket);
	return true;
}
