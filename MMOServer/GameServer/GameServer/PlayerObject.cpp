#include "stdafx.h"
#include "PlayerObject.h"
#include "MonsterObject.h"
#include "CristalObject.h"
#include "ObjectManager.h"
#include "GameContent.h"
#include "Packet.h"
#include "../../Common/MathUtil.h"
#include "../../Lib/Network/include/NetPacket.h"

using namespace Jay;

LFObjectPool_TLS<PlayerObject> PlayerObject::_pool(0, true);

PlayerObject::PlayerObject() : BaseObject(PLAYER), _login(false)
{
}
PlayerObject::~PlayerObject()
{
}
PlayerObject* PlayerObject::Alloc()
{
	return _pool.Alloc();
}
void PlayerObject::Free(PlayerObject* player)
{
	_pool.Free(player);
}
bool PlayerObject::Update()
{
	if (IsDeleted())
		return false;

	RecoverHP();

	return true;
}
void PlayerObject::Dispose()
{
	PlayerObject::Free(this);
}
void PlayerObject::Init(WCHAR* nickname, BYTE type, float posX, float posY, USHORT rotation, int cristal, int hp, INT64 exp, USHORT level, bool die)
{
	wcscpy_s(_nickname, nickname);
	_characterType = type;
	_posX = posX;
	_posY = posY;
	_tileX = _posX * 2;
	_tileY = _posY * 2;
	_curSector.x = _tileX / dfSECTOR_SIZE_X;
	_curSector.y = _tileY / dfSECTOR_SIZE_Y;
	_rotation = rotation;
	_cristal = cristal;
	_hp = hp;
	_exp = exp;
	_level = level;
	_die = die;
	_sit = false;
}
void PlayerObject::GameSetup(GameContent* game)
{
	_game = game;

	DATA_PLAYER* dataPlayer = _game->GetDataPlayer();
	_power = dataPlayer->damage;
	_maxHP = dataPlayer->hp;
	_recoveryHP = dataPlayer->recovery_hp;
}
INT64 PlayerObject::GetAccountNo()
{
	return _accountNo;
}
INT64 PlayerObject::GetSessionID()
{
	return _sessionID;
}
BYTE PlayerObject::GetCharacterType()
{
	return _characterType;
}
WCHAR* PlayerObject::GetNickname()
{
	return _nickname;
}
USHORT PlayerObject::GetRotation()
{
	return _rotation;
}
int PlayerObject::GetCristal()
{
	return _cristal;
}
int PlayerObject::GetHP()
{
	return _hp;
}
INT64 PlayerObject::GetExp()
{
	return _exp;
}
USHORT PlayerObject::GetLevel()
{
	return _level;
}
BYTE PlayerObject::GetVKey()
{
	return _vKey;
}
BYTE PlayerObject::GetHKey()
{
	return _hKey;
}
int PlayerObject::GetPower()
{
	return _power;
}
bool PlayerObject::IsDie()
{
	return _die;
}
bool PlayerObject::IsSit()
{
	return _sit;
}
bool PlayerObject::IsLogin()
{
	return _login;
}
void PlayerObject::Auth(INT64 sessionID, INT64 accountNo)
{
	_sessionID = sessionID;
	_accountNo = accountNo;
	_login = true;
}
bool PlayerObject::UpdateTile()
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
bool PlayerObject::UpdateSector()
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
		_game->UpdateSectorAround_Player(this);
		return true;
	}
	return false;
}
void PlayerObject::Restart()
{
	//--------------------------------------------------------------------
	// 플레이어 리스폰 정보 세팅
	//--------------------------------------------------------------------
	_tileX = (dfPLAYER_TILE_X_RESPAWN_CENTER - 10) + (rand() % 21);
	_tileY = (dfPLAYER_TILE_Y_RESPAWN_CENTER - 15) + (rand() % 31);
	_posX = (float)_tileX / 2;
	_posY = (float)_tileY / 2;
	_curSector.x = _tileX / dfSECTOR_SIZE_X;
	_curSector.y = _tileY / dfSECTOR_SIZE_Y;
	_rotation = rand() % 360;
	_hp = _maxHP;
	_sit = false;
	_die = false;

	//--------------------------------------------------------------------
	// 필드 및 섹터에 플레이어 삽입
	//--------------------------------------------------------------------
	_game->AddTile(this);
	_game->AddSector(this);

	//--------------------------------------------------------------------------------------
	// 1. 부활 플레이어에게 - 다시하기 패킷
	// 2. 부활 플레이어에게 - 부활 플레이어의 생성 패킷
	// 3. 부활 플레이어에게 - 현재섹터에 존재하는 오브젝트들의 생성 패킷
	// 4. 현재섹터에 존재하는 플레이어들에게 - 부활 플레이어의 생성 패킷
	//--------------------------------------------------------------------------------------

	//--------------------------------------------------------------------------------------
	// 1. 부활 플레이어에게 다시하기 패킷 보내기
	//--------------------------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeRestart(packet1);
	_game->SendUnicast(this, packet1);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------------------------
	// 2. 부활 플레이어에게 생성 패킷 보내기
	//--------------------------------------------------------------------------------------
	NetPacket* packet2 = NetPacket::Alloc();

	Packet::MakeCreateMyCharacter(packet2
		, _objectID
		, _characterType
		, _nickname
		, _posX
		, _posY
		, _rotation
		, _cristal
		, _hp
		, _exp
		, _level);
	_game->SendUnicast(this, packet2);

	NetPacket::Free(packet2);

	//--------------------------------------------------------------------------------------
	// 3. 부활 플레이어에게 현재섹터에 존재하는 오브젝트들의 생성 패킷 보내기
	//--------------------------------------------------------------------------------------
	SECTOR_AROUND sectorAround;
	_game->GetSectorAround(_curSector.x, _curSector.y, &sectorAround);

	std::list<BaseObject*>* sector;
	for (int i = 0; i < sectorAround.count; i++)
	{
		sector = _game->GetSector(sectorAround.around[i].x, sectorAround.around[i].y);
		for (auto iter = sector->begin(); iter != sector->end(); ++iter)
		{
			BaseObject* object = *iter;
			if (object == this)
				continue;

			switch (object->GetType())
			{
			case PLAYER:
				{
					PlayerObject* existPlayer = static_cast<PlayerObject*>(object);
					if (!existPlayer->IsDie())
					{
						NetPacket* packet3 = NetPacket::Alloc();

						Packet::MakeCreateOtherCharacter(packet3
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
						_game->SendUnicast(this, packet3);

						NetPacket::Free(packet3);
					}
				}
				break;
			case MONSTER:
				{
					MonsterObject* existMonster = static_cast<MonsterObject*>(object);

					NetPacket* packet3 = NetPacket::Alloc();

					Packet::MakeCreateMonster(packet3
						, existMonster->GetID()
						, existMonster->GetPosX()
						, existMonster->GetPosY()
						, existMonster->GetRotation()
						, FALSE);
					_game->SendUnicast(this, packet3);

					NetPacket::Free(packet3);
				}
				break;
			case CRISTAL:
				{
					CristalObject* existCristal = static_cast<CristalObject*>(object);

					NetPacket* packet3 = NetPacket::Alloc();

					Packet::MakeCreateCristal(packet3
						, existCristal->GetID()
						, existCristal->GetCristalType()
						, existCristal->GetPosX()
						, existCristal->GetPosY());
					_game->SendUnicast(this, packet3);

					NetPacket::Free(packet3);
				}
				break;
			default:
				break;
			}
		}
	}

	//--------------------------------------------------------------------------------------
	// 4. 현재섹터에 존재하는 플레이어들에게 부활 플레이어의 생성 패킷 보내기
	//--------------------------------------------------------------------------------------
	NetPacket* packet4 = NetPacket::Alloc();

	Packet::MakeCreateOtherCharacter(packet4
		, _objectID
		, _characterType
		, _nickname
		, _posX
		, _posY
		, _rotation
		, _level
		, TRUE
		, _sit
		, _die);
	_game->SendSectorAround(this, packet4, false);

	NetPacket::Free(packet4);

	//--------------------------------------------------------------------
	// DB 에 플레이어의 재시작 정보 저장
	//--------------------------------------------------------------------
	_game->DBPostPlayerRestart(this);
}
void PlayerObject::MoveStart(float posX, float posY, USHORT rotation, BYTE vKey, BYTE hKey)
{
	//--------------------------------------------------------------------
	// 플레이어 좌표 변경
	//--------------------------------------------------------------------
	_posX = posX;
	_posY = posY;

	//--------------------------------------------------------------------
	// 플레이어 방향 및 동작 변경
	//--------------------------------------------------------------------
	_rotation = rotation;
	_vKey = vKey;
	_hKey = hKey;

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 이동 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMoveCharacterStart(packet
		, _objectID
		, _posX
		, _posY
		, _rotation
		, _vKey
		, _hKey);
	_game->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// 플레이어의 이동 동작으로 좌표가 변경된 경우 타일 및 섹터 업데이트 처리
	//--------------------------------------------------------------------
	UpdateTile();
	UpdateSector();
}
void PlayerObject::MoveStop(float posX, float posY, USHORT rotation)
{
	_posX = posX;
	_posY = posY;
	_rotation = rotation;

	//--------------------------------------------------------------------
	// 플레이어가 앉아있던 경우 회복된 HP를 해당 플레이어에게 알리고 DB에 저장
	//--------------------------------------------------------------------
	if (_sit)
	{
		NetPacket* packet = NetPacket::Alloc();

		Packet::MakeSyncHP(packet, _hp);
		_game->SendUnicast(this, packet);

		NetPacket::Free(packet);

		int sitTimeSec = (timeGetTime() - _sitStartTime) / 1000;
		_game->DBPostRecoverHP(this, _oldHP, sitTimeSec);

		_sit = false;
	}

	//--------------------------------------------------------------------
	// 플레이어의 정지 동작으로 좌표가 변경된 경우 타일 및 섹터 업데이트 처리
	//--------------------------------------------------------------------
	UpdateTile();
	UpdateSector();

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 정지 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMoveCharacterStop(packet
		, _objectID
		, _posX
		, _posY
		, _rotation);
	_game->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);
}
void PlayerObject::Attack1()
{
	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 공격 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeAttack1(packet1, _objectID);
	_game->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// 전방 좌표에 있는 몬스터를 찾아 데미지 처리
	//--------------------------------------------------------------------
	MonsterObject* monster;
	if (FindObjectInFront(MONSTER, (BaseObject**)&monster))
	{
		NetPacket* packet2 = NetPacket::Alloc();
		
		Packet::MakeDamage(packet2
			, _objectID
			, monster->GetID()
			, _power);
		_game->SendSectorAround(monster, packet2, false);

		NetPacket::Free(packet2);

		monster->Damage(this, _power);
	}
}
void PlayerObject::Attack2()
{
	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 공격 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeAttack2(packet1, _objectID);
	_game->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// 전방 좌표에 있는 몬스터를 찾아 데미지 처리
	//--------------------------------------------------------------------
	MonsterObject* monster;
	if (FindObjectInFront(MONSTER, (BaseObject**)&monster))
	{
		NetPacket* packet2 = NetPacket::Alloc();

		Packet::MakeDamage(packet2
			, _objectID
			, monster->GetID()
			, _power);
		_game->SendSectorAround(monster, packet2, false);

		NetPacket::Free(packet2);

		monster->Damage(this, _power);
	}
}
void PlayerObject::Pick()
{
	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 먹기 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakePickCristal(packet1, _objectID);
	_game->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// 전방 좌표에 있는 크리스탈을 찾아 획득 처리
	//--------------------------------------------------------------------
	CristalObject* cristal;
	if (FindObjectInFront(CRISTAL, (BaseObject**)&cristal))
	{
		int amount = cristal->GetAmount();
		_cristal += amount;

		NetPacket* packet2 = NetPacket::Alloc();

		Packet::MakeGetCristal(packet2
			, _objectID
			, cristal->GetID()
			, _cristal);
		_game->SendSectorAround(this, packet2, true);

		NetPacket::Free(packet2);

		_game->DeleteCristal(cristal);
		_game->DBPostGetCristal(this, amount);
	}
}
void PlayerObject::Sit()
{
	DWORD currentTime = timeGetTime();
	_lastRecoverHPTime = currentTime;
	_sitStartTime = currentTime;
	_oldHP = _hp;
	_sit = true;

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 앉기 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeSitCharacter(packet1, _objectID);
	_game->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);
}
void PlayerObject::Damage(int damage)
{
	if (_hp > damage)
	{
		_hp -= damage;
		return;
	}

	//--------------------------------------------------------------------
	// 플레이어 사망에 따른 크리스탈 차감
	//--------------------------------------------------------------------
	int minusCristal = (_cristal - dfPLAYER_DIE_MINUS_CRISTAL > 0) ? dfPLAYER_DIE_MINUS_CRISTAL : _cristal;
	_cristal -= minusCristal;
	_hp = 0;
	_die = true;

	//--------------------------------------------------------------------
	// 필드에서 플레이어 제거
	//--------------------------------------------------------------------
	_game->RemoveTile(this);

	//--------------------------------------------------------------------
	// 해당 플레이어에게 사망 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeCharacterDie(packet1, _objectID, minusCristal);
	_game->SendUnicast(this, packet1);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 플레이어들에게 해당 플레이어의 삭제 패킷 보내기
	//--------------------------------------------------------------------
	NetPacket* packet2 = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet2, _objectID);
	_game->SendSectorAround(this, packet2, false);

	NetPacket::Free(packet2);

	//--------------------------------------------------------------------
	// DB에 해당 플레이어의 사망 정보 저장
	//--------------------------------------------------------------------
	_game->DBPostPlayerDie(this, minusCristal);
}
void PlayerObject::RecoverHP()
{
	//--------------------------------------------------------------------
	// 앉아있는 상태인지 확인
	//--------------------------------------------------------------------
	if (!_sit)
		return;

	//--------------------------------------------------------------------
	// HP 회복 쿨타임 확인
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _lastRecoverHPTime < dfPLAYER_HP_RECOVER_COOLING_TIME)
		return;

	if (_hp + _recoveryHP < _maxHP)
		_hp += _recoveryHP;
	else
		_hp = _maxHP;

	_lastRecoverHPTime += dfPLAYER_HP_RECOVER_COOLING_TIME;
}
bool PlayerObject::FindObjectInFront(OBJECT_TYPE type, BaseObject** object)
{
	//--------------------------------------------------------------------
	// 전방 좌표 계산
	//--------------------------------------------------------------------
	float degree = _rotation - 90;
	float leftRadian = DegreeToRadian(degree - 45);
	float frontRadian = DegreeToRadian(degree);
	float rightRadian = DegreeToRadian(degree + 45);

	int tileX = GetTileX();									// 현재 필드 X 좌표
	int tileY = GetTileY();									// 현재 필드 Y 좌표
	int leftX = tileX + round(std::cosf(leftRadian));		// 전방 왼쪽 필드 X 좌표
	int leftY = tileY - round(std::sinf(leftRadian));		// 전방 왼쪽 필드 Y 좌표
	int frontX = tileX + round(std::cosf(frontRadian));		// 전방 필드 X 좌표
	int frontY = tileY - round(std::sinf(frontRadian));		// 전방 필드 Y 좌표
	int rightX = tileX + round(std::cosf(rightRadian));		// 전방 오른쪽 필드 X 좌표
	int rightY = tileY - round(std::sinf(rightRadian));		// 전방 오른쪽 필드 Y 좌표

	int nextX[4] = { tileX, leftX, frontX, rightX };
	int nextY[4] = { tileY, leftY, frontY, rightY };

	//--------------------------------------------------------------------
	// 전방 좌표에 있는 오브젝트 찾기
	//--------------------------------------------------------------------
	std::list<BaseObject*>* tile;
	for (int i = 0; i < 4; i++)
	{
		if (_game->IsTileOut(nextX[i], nextY[i]))
			continue;

		tile = _game->GetTile(nextX[i], nextY[i]);
		for (auto iter = tile->begin(); iter != tile->end(); ++iter)
		{
			BaseObject* targetObject = *iter;
			if (targetObject->GetType() == type)
			{
				*object = targetObject;
				return true;
			}
		}
	}

	return false;
}
