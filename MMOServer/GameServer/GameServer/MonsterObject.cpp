#include "stdafx.h"
#include "MonsterObject.h"
#include "CristalObject.h"
#include "ObjectManager.h"
#include "GameServer.h"
#include "Packet.h"
#include "../../Lib/Network/include/NetPacket.h"
#include "../../Common/MathUtil.h"

using namespace Jay;

LFObjectPool_TLS<MonsterObject> MonsterObject::_pool(0, true);

MonsterObject::MonsterObject() : BaseObject(MONSTER), _release(false)
{
}
MonsterObject::~MonsterObject()
{
}
MonsterObject* MonsterObject::Alloc(GameServer* server)
{
	MonsterObject* monster = _pool.Alloc();
	monster->_server = server;
	monster->_lastDeadTime = 0;
	return monster;
}
void MonsterObject::Free(MonsterObject* monster)
{
	_pool.Free(monster);
}
bool MonsterObject::Update()
{
	if (_release)
		return false;

	Respawn();

	return true;
}
void MonsterObject::Dispose()
{
	MonsterObject::Free(this);
}
void MonsterObject::Initial(BYTE type, WORD spawnZone)
{
	_monsterType = type;
	_spawnZone = spawnZone;
	_die = true;
}
void MonsterObject::Release()
{
	_release = true;
}
BYTE MonsterObject::GetMonsterType()
{
	return _monsterType;
}
USHORT MonsterObject::GetRotation()
{
	return _rotation;
}
int MonsterObject::GetHP()
{
	return _hp;
}
bool MonsterObject::IsDie()
{
	return _die;
}
void MonsterObject::Respawn()
{
	//--------------------------------------------------------------------
	// 몬스터 생존 여부 확인
	//--------------------------------------------------------------------
	if (!_die)
		return;

	//--------------------------------------------------------------------
	// 몬스터 리스폰 쿨타임 확인
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _lastDeadTime < dfMONSTER_RESPAWN_TIME)
		return;

	//--------------------------------------------------------------------
	// 몬스터 데이터 초기화
	//--------------------------------------------------------------------
	DATA_MONSTER* dataMonster = _server->GetDataMonster(_monsterType);
	_hp = dataMonster->hp;
	_power = dataMonster->damage;
	_rotation = rand() % 360;
	_die = false;

	//--------------------------------------------------------------------
	// 몬스터 리스폰 좌표 설정
	//--------------------------------------------------------------------
	switch (_spawnZone)
	{
	case ZONE_1:
		_tileX = (dfMONSTER_TILE_X_RESPAWN_CENTER_1 - 10) + (rand() % 21);
		_tileY = (dfMONSTER_TILE_Y_RESPAWN_CENTER_1 - 15) + (rand() % 31);
		break;
	case ZONE_2:
		_tileX = (dfMONSTER_TILE_X_RESPAWN_CENTER_2 - 10) + (rand() % 21);
		_tileY = (dfMONSTER_TILE_Y_RESPAWN_CENTER_2 - 15) + (rand() % 31);
		break;
	case ZONE_3:
		_tileX = (dfMONSTER_TILE_X_RESPAWN_CENTER_3 - 10) + (rand() % 21);
		_tileY = (dfMONSTER_TILE_Y_RESPAWN_CENTER_3 - 15) + (rand() % 31);
		break;
	case ZONE_4:
		_tileX = (dfMONSTER_TILE_X_RESPAWN_CENTER_4 - 10) + (rand() % 21);
		_tileY = (dfMONSTER_TILE_Y_RESPAWN_CENTER_4 - 15) + (rand() % 31);
		break;
	case ZONE_5:
		_tileX = (dfMONSTER_TILE_X_RESPAWN_CENTER_5 - 10) + (rand() % 21);
		_tileY = (dfMONSTER_TILE_Y_RESPAWN_CENTER_5 - 15) + (rand() % 31);
		break;
	case ZONE_6:
		_tileX = (dfMONSTER_TILE_X_RESPAWN_CENTER_6 - 10) + (rand() % 21);
		_tileY = (dfMONSTER_TILE_Y_RESPAWN_CENTER_6 - 15) + (rand() % 31);
		break;
	case ZONE_7:
		_tileX = (dfMONSTER_TILE_X_RESPAWN_CENTER_7 - 10) + (rand() % 21);
		_tileY = (dfMONSTER_TILE_Y_RESPAWN_CENTER_7 - 15) + (rand() % 31);
		break;
	default:
		break;
	}

	_posX = (float)_tileX / 2;
	_posY = (float)_tileY / 2;
	_tileX = _posX * 2;
	_tileY = _posY * 2;
	_curSector.x = _tileX / dfSECTOR_SIZE_X;
	_curSector.y = _tileY / dfSECTOR_SIZE_Y;

	_server->AddTile(this);
	_server->AddSector(this);

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 몬스터의 생성 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeCreateMonster(packet
		, _objectID
		, _posX
		, _posY
		, _rotation
		, TRUE);
	_server->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);
}
bool MonsterObject::UpdateTile()
{
	//--------------------------------------------------------------------
	// 현재 위치한 타일에서 삭제
	// 현재의 좌표로 타일을 새로 계산하여 해당 타일에 추가
	//--------------------------------------------------------------------
	int tileX = _posX * 2;
	int tileY = _posY * 2;

	if (_tileX != tileX || _tileY != tileY)
	{
		_server->RemoveTile(this);

		_tileX = tileX;
		_tileY = tileY;

		_server->AddTile(this);
		return true;
	}
	return false;
}
bool MonsterObject::UpdateSector()
{
	//--------------------------------------------------------------------
	// 현재 위치한 섹터에서 삭제
	// 현재의 좌표로 섹터를 새로 계산하여 해당 섹터에 추가
	//--------------------------------------------------------------------
	int sectorX = GetTileX() / dfSECTOR_SIZE_X;
	int sectorY = GetTileY() / dfSECTOR_SIZE_Y;

	if (_curSector.x != sectorX || _curSector.y != sectorY)
	{
		_server->RemoveSector(this);

		_oldSector.x = _curSector.x;
		_oldSector.y = _curSector.y;
		_curSector.x = sectorX;
		_curSector.y = sectorY;

		_server->AddSector(this);
		_server->UpdateSectorAround_Monster(this);
		return true;
	}
	return false;
}
void MonsterObject::Move(float targetX, float targetY)
{
	_targetX = targetX;
	_targetY = targetY;

	_radian = atan2f(_targetY - _posY, _targetX - _posX);
	_rotation = round(Jay::RadianToDegree(_radian)) + 90;
}
void MonsterObject::Damage(int damage)
{
	if (_hp > damage)
	{
		_hp -= damage;
		return;
	}

	_hp = 0;
	_die = true;
	_lastDeadTime = timeGetTime();

	//--------------------------------------------------------------------
	// 필드 및 섹터에서 몬스터 제거
	//--------------------------------------------------------------------
	_server->RemoveTile(this);
	_server->RemoveSector(this);

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 몬스터의 사망 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMonsterDie(packet, _objectID);
	_server->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// 몬스터가 사망한 위치에 크리스탈 생성
	//--------------------------------------------------------------------
	_server->CreateCristal(_posX, _posY);
}
