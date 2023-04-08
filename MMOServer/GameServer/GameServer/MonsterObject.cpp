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
	// ���� ��ġ�� Ÿ�Ͽ��� ����
	// ������ ��ǥ�� Ÿ���� ���� ����Ͽ� �ش� Ÿ�Ͽ� �߰�
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
	// ���� ��ġ�� ���Ϳ��� ����
	// ������ ��ǥ�� ���͸� ���� ����Ͽ� �ش� ���Ϳ� �߰�
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
	// ���� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (!_die)
		return;

	//--------------------------------------------------------------------
	// ���� ������ ��Ÿ�� Ȯ��
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _lastDeadTime < dfMONSTER_RESPAWN_TIME)
		return;

	//--------------------------------------------------------------------
	// ���� ������ �ʱ�ȭ
	//--------------------------------------------------------------------
	_hp = _maxHP;
	_rotation = rand() % 360;
	_lastMoveTime = 0;
	_lastAttackTime = 0;
	_targetPlayer = nullptr;
	_die = false;

	//--------------------------------------------------------------------
	// ���� ������ ��ǥ ����
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
	// Ÿ�� �� ���Ϳ� �ش� ���� �߰�
	//--------------------------------------------------------------------
	_game->AddTile(this);
	_game->AddSector(this);

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� ������ ���� ��Ŷ ������
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
	// ���� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (_die)
		return;

	//--------------------------------------------------------------------
	// ���� �̵� ��Ÿ�� Ȯ��
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _lastMoveTime < dfMONSTER_MOVE_COOLING_TIME)
		return;

	//--------------------------------------------------------------------
	// ���Ͱ� Ÿ�� �÷��̾�� �̵��ؾ��ϴ��� Ȯ��
	//--------------------------------------------------------------------
	float posX;
	float posY;
	float radian;
	int degree;
	int chance;

	if (IsTargetVisibleAndInRange() && !IsAttackInRange())
	{
		// ������ ����� �����Ƿ� Ÿ�� �÷��̾� �������� �̵�
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
		// ������ ����� �����Ƿ� ���� Ȯ���� ���� ���� �������� �̵�
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
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� ������ �̵� ��Ŷ ������
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
	// Ÿ�� �� ���� ������Ʈ ó��
	//--------------------------------------------------------------------
	UpdateTile();
	UpdateSector();

	//--------------------------------------------------------------------
	// ���� �̵� �ð� ����
	//--------------------------------------------------------------------
	_lastMoveTime += dfMONSTER_MOVE_COOLING_TIME;
}
void MonsterObject::Attack()
{
	//--------------------------------------------------------------------
	// ���� ���� ���� Ȯ��
	//--------------------------------------------------------------------
	if (_die)
		return;

	//--------------------------------------------------------------------
	// ���� ���� ��Ÿ�� Ȯ��
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _lastAttackTime < dfMONSTER_ATTACK_COOLING_TIME)
		return;

	//--------------------------------------------------------------------
	// ������ Ÿ�� �÷��̾ ���� ������ν� ��ȿ���� Ȯ��
	//--------------------------------------------------------------------
	if (!IsTargetVisibleAndInRange())
		return;

	//--------------------------------------------------------------------
	// ������ ���� ��Ÿ� ���� Ÿ�� �÷��̾ �ִ��� Ȯ��
	//--------------------------------------------------------------------
	if (!IsAttackInRange())
		return;

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� ������ ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeMonsterAttack(packet1, _objectID);
	_game->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� ������ ������ ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet2 = NetPacket::Alloc();

	Packet::MakeDamage(packet2, _objectID, _targetPlayer->GetID(), _power);
	_game->SendSectorAround(this, packet2, false);

	NetPacket::Free(packet2);

	//--------------------------------------------------------------------
	// Ÿ�� �÷��̾�� ������ ���� �˸�
	//--------------------------------------------------------------------
	_targetPlayer->Damage(_power);

	//--------------------------------------------------------------------
	// ���� ���� �ð� ����
	//--------------------------------------------------------------------
	_lastAttackTime += dfMONSTER_ATTACK_COOLING_TIME;
}
void MonsterObject::Damage(PlayerObject* attackPlayer, int damage)
{
	if (_hp > damage)
	{
		_hp -= damage;

		// ������ �÷��̾� ������ �����Ͽ� �ξ��ٰ� ã�ư��� �����Ѵ�.
		_targetPlayerID = attackPlayer->GetID();
		_targetPlayer = attackPlayer;
		_lastAttackTime = timeGetTime();
		return;
	}

	_hp = 0;
	_die = true;
	_lastDeadTime = timeGetTime();

	//--------------------------------------------------------------------
	// �ʵ� �� ���Ϳ��� ���� ����
	//--------------------------------------------------------------------
	_game->RemoveTile(this);
	_game->RemoveSector(this);

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� ������ ��� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMonsterDie(packet, _objectID);
	_game->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// ���Ͱ� ����� ��ġ�� ũ����Ż ����
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
	// ������ �÷��̾ ���°��
	//--------------------------------------------------------------------
	if (_targetPlayer == nullptr)
		return false;

	//--------------------------------------------------------------------
	// ������ �÷��̾� ��ü�� ������ ���
	//--------------------------------------------------------------------
	if (_targetPlayer->IsDeleted())
	{
		_targetPlayer = nullptr;
		return false;
	}

	//--------------------------------------------------------------------
	// ������ �÷��̾� ��ü�� ������ƮǮ�� ���� ����� ���
	//--------------------------------------------------------------------
	if (_targetPlayer->GetID() != _targetPlayerID)
	{
		_targetPlayer = nullptr;
		return false;
	}

	//--------------------------------------------------------------------
	// ������ �÷��̾ ����� ���
	//--------------------------------------------------------------------
	if (_targetPlayer->IsDie())
	{
		_targetPlayer = nullptr;
		return false;
	}

	//--------------------------------------------------------------------
	// ������ �÷��̾ ���� Ȱ������ ������ ���� ���
	//--------------------------------------------------------------------
	if (!IsInMonsterZone(_targetPlayer->GetTileX(), _targetPlayer->GetTileY()))
	{
		_targetPlayer = nullptr;
		return false;
	}

	//--------------------------------------------------------------------
	// ������ �÷��̾ ���� �þ߹��� ������ ���� ���
	//--------------------------------------------------------------------
	if (!IsSightInRange(_targetPlayer->GetTileX(), _targetPlayer->GetTileY()))
	{
		_targetPlayer = nullptr;
		return false;
	}

	return true;
}
