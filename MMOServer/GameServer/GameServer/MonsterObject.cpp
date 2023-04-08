#include "stdafx.h"
#include "MonsterObject.h"
#include "CristalObject.h"
#include "ObjectManager.h"
#include "GameContent.h"
#include "Packet.h"
#include "../../Lib/Network/include/NetPacket.h"
#include "../../Common/MathUtil.h"

using namespace Jay;

LFObjectPool_TLS<MonsterObject> MonsterObject::_pool(0, true);

MonsterObject::MonsterObject() : BaseObject(MONSTER)
{
}
MonsterObject::~MonsterObject()
{
}
MonsterObject* MonsterObject::Alloc(GameContent* game)
{
	MonsterObject* monster = _pool.Alloc();
	monster->_game = game;
	monster->_lastDeadTime = 0;
	return monster;
}
void MonsterObject::Free(MonsterObject* monster)
{
	_pool.Free(monster);
}
bool MonsterObject::Update()
{
	if (IsDeleted())
		return false;

	Respawn();
	Move();
	Attack();

	return true;
}
void MonsterObject::Dispose()
{
	MonsterObject::Free(this);
}
void MonsterObject::Init(BYTE type, WORD spawnZone)
{
	_monsterType = type;
	_spawnZone = spawnZone;
	_die = true;
	_scala = sqrt(pow(dfMONSTER_SPEED_X, 2) + pow(dfMONSTER_SPEED_Y, 2));

	DATA_MONSTER* dataMonster = _game->GetDataMonster(_monsterType);
	_power = dataMonster->damage;
	_maxHP = dataMonster->hp;
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
		_game->RemoveTile(this);

		_tileX = tileX;
		_tileY = tileY;

		_game->AddTile(this);
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
		_game->RemoveSector(this);

		_oldSector.x = _curSector.x;
		_oldSector.y = _curSector.y;
		_curSector.x = sectorX;
		_curSector.y = sectorY;

		_game->AddSector(this);
		_game->UpdateSectorAround_Monster(this);
		return true;
	}
	return false;
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
	_hp = _maxHP;
	_rotation = rand() % 360;
	_lastMoveTime = 0;
	_lastAttackTime = 0;
	_targetPlayer = nullptr;
	_die = false;

	//--------------------------------------------------------------------
	// 몬스터 리스폰 좌표 설정
	//--------------------------------------------------------------------
	switch (_spawnZone)
	{
	case ZONE_1:
		_tileX = (dfMONSTER_TILE_X_RESPAWN_CENTER_1 - 10) + (rand() % 21);
		_tileY = (dfMONSTER_TILE_Y_RESPAWN_CENTER_1 - 10) + (rand() % 21);
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
	_curSector.x = _tileX / dfSECTOR_SIZE_X;
	_curSector.y = _tileY / dfSECTOR_SIZE_Y;

	//--------------------------------------------------------------------
	// 타일 및 섹터에 해당 몬스터 추가
	//--------------------------------------------------------------------
	_game->AddTile(this);
	_game->AddSector(this);

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
	_game->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);
}
void MonsterObject::Move()
{
	//--------------------------------------------------------------------
	// 몬스터 생존 여부 확인
	//--------------------------------------------------------------------
	if (_die)
		return;

	//--------------------------------------------------------------------
	// 몬스터 이동 쿨타임 확인
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _lastMoveTime < dfMONSTER_MOVE_COOLING_TIME)
		return;

	//--------------------------------------------------------------------
	// 몬스터가 타겟 플레이어에게 이동해야하는지 확인
	//--------------------------------------------------------------------
	float posX;
	float posY;
	float radian;
	int degree;
	int chance;

	if (IsTargetVisibleAndInRange() && !IsAttackInRange())
	{
		// 공격할 대상이 있으므로 타겟 플레이어 방향으로 이동
		posX = _targetPlayer->GetPosX() - _posX;
		posY = _targetPlayer->GetPosY() - _posY;
		radian = atan2f(posY, posX);
		degree = RadianToDegree(radian);

		_posX += cosf(radian) * _scala;
		_posY += sinf(radian) * _scala;
		_rotation = (360 - degree + 90) % 360;
	}
	else
	{
		// 공격할 대상이 없으므로 일정 확률에 의해 랜덤 방향으로 이동
		chance = rand() % 100;
		if (chance >= dfMONSTER_MOVE_RANDOM_CHANCE)
			return;

		degree = rand() % 360;
		radian = DegreeToRadian(degree);
		posX = _posX + (cosf(radian) * _scala);
		posY = _posY + (sinf(radian) * _scala);

		if (!IsInMonsterZone(posX * 2, posY * 2))
			return;

		_posX = posX;
		_posY = posY;
		_rotation = (360 - degree + 90) % 360;
	}

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 몬스터의 이동 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMoveMonster(packet
		, _objectID
		, _posX
		, _posY
		, _rotation);
	_game->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// 타일 및 섹터 업데이트 처리
	//--------------------------------------------------------------------
	UpdateTile();
	UpdateSector();

	//--------------------------------------------------------------------
	// 몬스터 이동 시간 갱신
	//--------------------------------------------------------------------
	_lastMoveTime += dfMONSTER_MOVE_COOLING_TIME;
}
void MonsterObject::Attack()
{
	//--------------------------------------------------------------------
	// 몬스터 생존 여부 확인
	//--------------------------------------------------------------------
	if (_die)
		return;

	//--------------------------------------------------------------------
	// 몬스터 공격 쿨타임 확인
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _lastAttackTime < dfMONSTER_ATTACK_COOLING_TIME)
		return;

	//--------------------------------------------------------------------
	// 몬스터의 타겟 플레이어가 공격 대상으로써 유효한지 확인
	//--------------------------------------------------------------------
	if (!IsTargetVisibleAndInRange())
		return;

	//--------------------------------------------------------------------
	// 몬스터의 공격 사거리 내에 타겟 플레이어가 있는지 확인
	//--------------------------------------------------------------------
	if (!IsAttackInRange())
		return;

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 몬스터의 공격 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeMonsterAttack(packet1, _objectID);
	_game->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 몬스터의 데미지 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet2 = NetPacket::Alloc();

	Packet::MakeDamage(packet2, _objectID, _targetPlayer->GetID(), _power);
	_game->SendSectorAround(this, packet2, false);

	NetPacket::Free(packet2);

	//--------------------------------------------------------------------
	// 타겟 플레이어에게 몬스터의 공격 알림
	//--------------------------------------------------------------------
	_targetPlayer->Damage(_power);

	//--------------------------------------------------------------------
	// 몬스터 공격 시간 갱신
	//--------------------------------------------------------------------
	_lastAttackTime += dfMONSTER_ATTACK_COOLING_TIME;
}
void MonsterObject::Damage(PlayerObject* attackPlayer, int damage)
{
	if (_hp > damage)
	{
		_hp -= damage;

		// 공격한 플레이어 정보를 저장하여 두었다가 찾아가서 공격한다.
		_targetPlayerID = attackPlayer->GetID();
		_targetPlayer = attackPlayer;
		_lastAttackTime = timeGetTime();
		return;
	}

	_hp = 0;
	_die = true;
	_lastDeadTime = timeGetTime();

	//--------------------------------------------------------------------
	// 필드 및 섹터에서 몬스터 제거
	//--------------------------------------------------------------------
	_game->RemoveTile(this);
	_game->RemoveSector(this);

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 몬스터의 사망 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMonsterDie(packet, _objectID);
	_game->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// 몬스터가 사망한 위치에 크리스탈 생성
	//--------------------------------------------------------------------
	_game->CreateCristal(_posX, _posY);
}
bool MonsterObject::IsInMonsterZone(int tileX, int tileY)
{
	switch (_spawnZone)
	{
	case ZONE_1:
		if (tileX < dfMONSTER_TILE_X_RESPAWN_CENTER_1 - dfMONSTER_TILE_RESPAWN_RANGE_1 ||
			tileX > dfMONSTER_TILE_X_RESPAWN_CENTER_1 + dfMONSTER_TILE_RESPAWN_RANGE_1 ||
			tileY < dfMONSTER_TILE_Y_RESPAWN_CENTER_1 - dfMONSTER_TILE_RESPAWN_RANGE_1 ||
			tileY > dfMONSTER_TILE_Y_RESPAWN_CENTER_1 + dfMONSTER_TILE_RESPAWN_RANGE_1)
		{
			return false;
		}
		break;
	case ZONE_2:
		if (tileX < dfMONSTER_TILE_X_RESPAWN_CENTER_2 - dfMONSTER_TILE_RESPAWN_RANGE_2 ||
			tileX > dfMONSTER_TILE_X_RESPAWN_CENTER_2 + dfMONSTER_TILE_RESPAWN_RANGE_2 ||
			tileY < dfMONSTER_TILE_Y_RESPAWN_CENTER_2 - dfMONSTER_TILE_RESPAWN_RANGE_2 ||
			tileY > dfMONSTER_TILE_Y_RESPAWN_CENTER_2 + dfMONSTER_TILE_RESPAWN_RANGE_2)
		{
			return false;
		}
		break;
	case ZONE_3:
		if (tileX < dfMONSTER_TILE_X_RESPAWN_CENTER_3 - dfMONSTER_TILE_RESPAWN_RANGE_3 ||
			tileX > dfMONSTER_TILE_X_RESPAWN_CENTER_3 + dfMONSTER_TILE_RESPAWN_RANGE_3 ||
			tileY < dfMONSTER_TILE_Y_RESPAWN_CENTER_3 - dfMONSTER_TILE_RESPAWN_RANGE_3 ||
			tileY > dfMONSTER_TILE_Y_RESPAWN_CENTER_3 + dfMONSTER_TILE_RESPAWN_RANGE_3)
		{
			return false;
		}
		break;
	case ZONE_4:
		if (tileX < dfMONSTER_TILE_X_RESPAWN_CENTER_4 - dfMONSTER_TILE_RESPAWN_RANGE_4 ||
			tileX > dfMONSTER_TILE_X_RESPAWN_CENTER_4 + dfMONSTER_TILE_RESPAWN_RANGE_4 ||
			tileY < dfMONSTER_TILE_Y_RESPAWN_CENTER_4 - dfMONSTER_TILE_RESPAWN_RANGE_4 ||
			tileY > dfMONSTER_TILE_Y_RESPAWN_CENTER_4 + dfMONSTER_TILE_RESPAWN_RANGE_4)
		{
			return false;
		}
		break;
	case ZONE_5:
		if (tileX < dfMONSTER_TILE_X_RESPAWN_CENTER_5 - dfMONSTER_TILE_RESPAWN_RANGE_5 ||
			tileX > dfMONSTER_TILE_X_RESPAWN_CENTER_5 + dfMONSTER_TILE_RESPAWN_RANGE_5 ||
			tileY < dfMONSTER_TILE_Y_RESPAWN_CENTER_5 - dfMONSTER_TILE_RESPAWN_RANGE_5 ||
			tileY > dfMONSTER_TILE_Y_RESPAWN_CENTER_5 + dfMONSTER_TILE_RESPAWN_RANGE_5)
		{
			return false;
		}
		break;
	case ZONE_6:
		if (tileX < dfMONSTER_TILE_X_RESPAWN_CENTER_6 - dfMONSTER_TILE_RESPAWN_RANGE_6 ||
			tileX > dfMONSTER_TILE_X_RESPAWN_CENTER_6 + dfMONSTER_TILE_RESPAWN_RANGE_6 ||
			tileY < dfMONSTER_TILE_Y_RESPAWN_CENTER_6 - dfMONSTER_TILE_RESPAWN_RANGE_6 ||
			tileY > dfMONSTER_TILE_Y_RESPAWN_CENTER_6 + dfMONSTER_TILE_RESPAWN_RANGE_6)
		{
			return false;
		}
		break;
	case ZONE_7:
		if (tileX < dfMONSTER_TILE_X_RESPAWN_CENTER_7 - dfMONSTER_TILE_RESPAWN_RANGE_7 ||
			tileX > dfMONSTER_TILE_X_RESPAWN_CENTER_7 + dfMONSTER_TILE_RESPAWN_RANGE_7 ||
			tileY < dfMONSTER_TILE_Y_RESPAWN_CENTER_7 - dfMONSTER_TILE_RESPAWN_RANGE_7 ||
			tileY > dfMONSTER_TILE_Y_RESPAWN_CENTER_7 + dfMONSTER_TILE_RESPAWN_RANGE_7)
		{
			return false;
		}
		break;
	default:
		return false;
	}
	return true;
}
bool MonsterObject::IsSightInRange(int tileX, int tileY)
{
	if (abs(_tileX - _targetPlayer->GetTileX()) > dfMONSTER_SIGHT_RANGE_X ||
		abs(_tileY - _targetPlayer->GetTileY()) > dfMONSTER_SIGHT_RANGE_Y)
	{
		return false;
	}
	return true;
}
bool MonsterObject::IsAttackInRange()
{
	if (abs(_tileX - _targetPlayer->GetTileX()) > dfMONSTER_ATTACK_RANGE_X ||
		abs(_tileY - _targetPlayer->GetTileY()) > dfMONSTER_ATTACK_RANGE_Y)
	{
		return false;
	}
	return true;
}
bool MonsterObject::IsTargetVisibleAndInRange()
{
	//--------------------------------------------------------------------
	// 공격한 플레이어가 없는경우
	//--------------------------------------------------------------------
	if (_targetPlayer == nullptr)
		return false;

	//--------------------------------------------------------------------
	// 공격한 플레이어 객체가 삭제된 경우
	//--------------------------------------------------------------------
	if (_targetPlayer->IsDeleted())
	{
		_targetPlayer = nullptr;
		return false;
	}

	//--------------------------------------------------------------------
	// 공격한 플레이어 객체가 오브젝트풀에 의해 재사용된 경우
	//--------------------------------------------------------------------
	if (_targetPlayer->GetID() != _targetPlayerID)
	{
		_targetPlayer = nullptr;
		return false;
	}

	//--------------------------------------------------------------------
	// 공격한 플레이어가 사망한 경우
	//--------------------------------------------------------------------
	if (_targetPlayer->IsDie())
	{
		_targetPlayer = nullptr;
		return false;
	}

	//--------------------------------------------------------------------
	// 공격한 플레이어가 몬스터 활동구역 밖으로 나간 경우
	//--------------------------------------------------------------------
	if (!IsInMonsterZone(_targetPlayer->GetTileX(), _targetPlayer->GetTileY()))
	{
		_targetPlayer = nullptr;
		return false;
	}

	//--------------------------------------------------------------------
	// 공격한 플레이어가 몬스터 시야범위 밖으로 나간 경우
	//--------------------------------------------------------------------
	if (!IsSightInRange(_targetPlayer->GetTileX(), _targetPlayer->GetTileY()))
	{
		_targetPlayer = nullptr;
		return false;
	}

	return true;
}
