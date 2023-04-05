#include "stdafx.h"
#include "PlayerObject.h"
#include "MonsterObject.h"
#include "CristalObject.h"
#include "ObjectManager.h"
#include "GameServer.h"
#include "Packet.h"
#include "../../Common/MathUtil.h"
#include "../../Lib/Network/include/NetPacket.h"
#pragma comment(lib, "../../Lib/Redis/lib64/tacopie.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/cpp_redis.lib")

using namespace Jay;

LFObjectPool_TLS<PlayerObject> PlayerObject::_pool(0, true);

PlayerObject::PlayerObject() : BaseObject(PLAYER), _release(false), _login(false)
{
}
PlayerObject::~PlayerObject()
{
}
PlayerObject* PlayerObject::Alloc()
{
	PlayerObject* player = _pool.Alloc();
	return _pool.Alloc();
}
void PlayerObject::Free(PlayerObject* player)
{
	_pool.Free(player);
}
bool PlayerObject::Update()
{
	if (_release)
		return false;

	return true;
}
void PlayerObject::Dispose()
{
	PlayerObject::Free(this);
}
void PlayerObject::Initial(WCHAR* nickname, BYTE type, float posX, float posY, USHORT rotation, int cristal, int hp, INT64 exp, USHORT level, bool die)
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
void PlayerObject::Release()
{
	_release = true;
}
void PlayerObject::SetGameServer(GameServer* server)
{
	_server = server;
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
	// ���� ��ġ�� Ÿ�Ͽ��� ����
	// ������ ��ǥ�� Ÿ���� ���� ����Ͽ� �ش� Ÿ�Ͽ� �߰�
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
bool PlayerObject::UpdateSector()
{
	//--------------------------------------------------------------------
	// ���� ��ġ�� ���Ϳ��� ����
	// ������ ��ǥ�� ���͸� ���� ����Ͽ� �ش� ���Ϳ� �߰�
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
		_server->UpdateSectorAround_Player(this);
		return true;
	}
	return false;
}
void PlayerObject::Restart()
{
	//--------------------------------------------------------------------
	// �÷��̾� ������ ���� ����
	//--------------------------------------------------------------------
	DATA_PLAYER* dataPlayer = _server->GetDataPlayer();
	_tileX = (dfPLAYER_TILE_X_RESPAWN_CENTER - 10) + (rand() % 21);
	_tileY = (dfPLAYER_TILE_Y_RESPAWN_CENTER - 15) + (rand() % 31);
	_posX = (float)_tileX / 2;
	_posY = (float)_tileY / 2;
	_curSector.x = _tileX / dfSECTOR_SIZE_X;
	_curSector.y = _tileY / dfSECTOR_SIZE_Y;
	_rotation = rand() % 360;
	_hp = dataPlayer->hp;
	_sit = false;
	_die = false;

	//--------------------------------------------------------------------
	// �ʵ� �� ���Ϳ� �÷��̾� ����
	//--------------------------------------------------------------------
	_server->AddTile(this);
	_server->AddSector(this);

	//--------------------------------------------------------------------------------------
	// 1. ��Ȱ �÷��̾�� - �ٽ��ϱ� ��Ŷ
	// 2. ��Ȱ �÷��̾�� - ��Ȱ �÷��̾��� ���� ��Ŷ
	// 3. ��Ȱ �÷��̾�� - ���缽�Ϳ� �����ϴ� ������Ʈ���� ���� ��Ŷ
	// 4. ���缽�Ϳ� �����ϴ� �÷��̾�鿡�� - ��Ȱ �÷��̾��� ���� ��Ŷ
	//--------------------------------------------------------------------------------------

	//--------------------------------------------------------------------------------------
	// 1. ��Ȱ �÷��̾�� �ٽ��ϱ� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeRestart(packet1);
	_server->SendUnicast(this, packet1);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------------------------
	// 2. ��Ȱ �÷��̾�� ���� ��Ŷ ������
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
	_server->SendUnicast(this, packet2);

	NetPacket::Free(packet2);

	//--------------------------------------------------------------------------------------
	// 3. ��Ȱ �÷��̾�� ���缽�Ϳ� �����ϴ� ������Ʈ���� ���� ��Ŷ ������
	//--------------------------------------------------------------------------------------
	SECTOR_AROUND sectorAround;
	_server->GetSectorAround(_curSector.x, _curSector.y, &sectorAround);

	std::list<BaseObject*>* sector;
	for (int i = 0; i < sectorAround.count; i++)
	{
		sector = _server->GetSector(sectorAround.around[i].x, sectorAround.around[i].y);
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
						_server->SendUnicast(this, packet3);

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
					_server->SendUnicast(this, packet3);

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
					_server->SendUnicast(this, packet3);

					NetPacket::Free(packet3);
				}
				break;
			default:
				break;
			}
		}
	}

	//--------------------------------------------------------------------------------------
	// 4. ���缽�Ϳ� �����ϴ� �÷��̾�鿡�� ��Ȱ �÷��̾��� ���� ��Ŷ ������
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
	_server->SendSectorAround(this, packet4, false);

	NetPacket::Free(packet4);

	//--------------------------------------------------------------------
	// DB �� �÷��̾��� ����� ���� ����
	//--------------------------------------------------------------------
	_server->DBPostPlayerRestart(this);
}
void PlayerObject::MoveStart(float posX, float posY, USHORT rotation, BYTE vKey, BYTE hKey)
{
	//--------------------------------------------------------------------
	// �÷��̾� ��ǥ ����
	//--------------------------------------------------------------------
	_posX = posX;
	_posY = posY;

	//--------------------------------------------------------------------
	// �÷��̾� ���� �� ���� ����
	//--------------------------------------------------------------------
	_rotation = rotation;
	_vKey = vKey;
	_hKey = hKey;

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� �̵� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMoveCharacterStart(packet
		, _objectID
		, _posX
		, _posY
		, _rotation
		, _vKey
		, _hKey);
	_server->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);

	//--------------------------------------------------------------------
	// �÷��̾��� �̵� �������� ��ǥ�� ����� ��� Ÿ�� �� ���� ������Ʈ ó��
	//--------------------------------------------------------------------
	UpdateTile();
	UpdateSector();
}
void PlayerObject::MoveStop(float posX, float posY, USHORT rotation)
{
	//--------------------------------------------------------------------
	// �÷��̾� ��ǥ ����
	//--------------------------------------------------------------------
	_posX = posX;
	_posY = posY;

	//--------------------------------------------------------------------
	// �÷��̾��� ���� �������� ��ǥ�� ����� ��� Ÿ�� �� ���� ������Ʈ ó��
	//--------------------------------------------------------------------
	UpdateTile();
	UpdateSector();

	//--------------------------------------------------------------------
	// �÷��̾� ���� �� ���� ����
	//--------------------------------------------------------------------
	_rotation = rotation;
	_sit = false;

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeMoveCharacterStop(packet
		, _objectID
		, _posX
		, _posY
		, _rotation);
	_server->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);
}
void PlayerObject::Attack1()
{
	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeAttack1(packet1, _objectID);
	_server->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// ���� ��ǥ�� �ִ� ���͸� ã�� ������ ó��
	//--------------------------------------------------------------------
	MonsterObject* monster;
	if (FindForwardDirectionObject((BaseObject**)&monster, MONSTER))
	{
		DATA_PLAYER* dataPlayer = _server->GetDataPlayer();

		NetPacket* packet2 = NetPacket::Alloc();
		
		Packet::MakeDamage(packet2
			, _objectID
			, monster->GetID()
			, dataPlayer->damage);
		_server->SendSectorAround(monster, packet2, false);

		NetPacket::Free(packet2);

		monster->Damage(dataPlayer->damage);
	}
}
void PlayerObject::Attack2()
{
	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakeAttack2(packet1, _objectID);
	_server->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// ���� ��ǥ�� �ִ� ���͸� ã�� ������ ó��
	//--------------------------------------------------------------------
	MonsterObject* monster;
	if (FindForwardDirectionObject((BaseObject**)&monster, MONSTER))
	{
		DATA_PLAYER* dataPlayer = _server->GetDataPlayer();

		NetPacket* packet2 = NetPacket::Alloc();

		Packet::MakeDamage(packet2
			, _objectID
			, monster->GetID()
			, dataPlayer->damage);
		_server->SendSectorAround(monster, packet2, false);

		NetPacket::Free(packet2);

		monster->Damage(dataPlayer->damage);
	}
}
void PlayerObject::Pick()
{
	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� �Ա� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet1 = NetPacket::Alloc();

	Packet::MakePickCristal(packet1, _objectID);
	_server->SendSectorAround(this, packet1, false);

	NetPacket::Free(packet1);

	//--------------------------------------------------------------------
	// ���� ��ǥ�� �ִ� ũ����Ż�� ã�� ȹ�� ó��
	//--------------------------------------------------------------------
	CristalObject* cristal;
	if (FindForwardDirectionObject((BaseObject**)&cristal, CRISTAL))
	{
		DATA_CRISTAL* dataCristal = _server->GetDataCristal(cristal->GetCristalType());
		_cristal += dataCristal->amount;

		NetPacket* packet2 = NetPacket::Alloc();

		Packet::MakeGetCristal(packet2
			, _objectID
			, cristal->GetID()
			, _cristal);
		_server->SendSectorAround(this, packet2, true);

		NetPacket::Free(packet2);

		_server->DeleteCristal(cristal);
		_server->DBPostGetCristal(this, dataCristal->amount);
	}
}
void PlayerObject::Sit()
{
	_sit = !_sit;

	if (_sit)
	{
		//--------------------------------------------------------------------
		// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� �ɱ� ��Ŷ ������
		//--------------------------------------------------------------------
		NetPacket* packet1 = NetPacket::Alloc();

		Packet::MakeSitCharacter(packet1, _objectID);
		_server->SendSectorAround(this, packet1, false);

		NetPacket::Free(packet1);
	}
}
void PlayerObject::Damage(int damage)
{
	if (_hp > damage)
	{
		_hp -= damage;
		return;
	}

	_hp = 0;
	_die = true;

	//--------------------------------------------------------------------
	// �ʵ忡�� �÷��̾� ����
	//--------------------------------------------------------------------
	_server->RemoveTile(this);

	//--------------------------------------------------------------------
	// �ֺ� ����� ���Ϳ� �ִ� �÷��̾�鿡�� �ش� �÷��̾��� ���� ��Ŷ ������
	//--------------------------------------------------------------------
	NetPacket* packet = NetPacket::Alloc();

	Packet::MakeDeleteObject(packet, _objectID);
	_server->SendSectorAround(this, packet, false);

	NetPacket::Free(packet);
}
bool PlayerObject::FindForwardDirectionObject(BaseObject** object, OBJECT_TYPE type)
{
	//--------------------------------------------------------------------
	// ���� ��ǥ ���
	//--------------------------------------------------------------------
	double degree = _rotation - 90;
	double leftRadian = DegreeToRadian(degree - 45);
	double frontRadian = DegreeToRadian(degree);
	double rightRadian = DegreeToRadian(degree + 45);

	int tileX = GetTileX();									// ���� �ʵ� X ��ǥ
	int tileY = GetTileY();									// ���� �ʵ� Y ��ǥ
	int leftX = tileX + round(std::cos(leftRadian));		// ���� ���� �ʵ� X ��ǥ
	int leftY = tileY - round(std::sin(leftRadian));		// ���� ���� �ʵ� Y ��ǥ
	int frontX = tileX + round(std::cos(frontRadian));		// ���� �ʵ� X ��ǥ
	int frontY = tileY - round(std::sin(frontRadian));		// ���� �ʵ� Y ��ǥ
	int rightX = tileX + round(std::cos(rightRadian));		// ���� ������ �ʵ� X ��ǥ
	int rightY = tileY - round(std::sin(rightRadian));		// ���� ������ �ʵ� Y ��ǥ

	int dx[4] = { tileX, leftX, frontX, rightX };
	int dy[4] = { tileY, leftY, frontY, rightY };

	//--------------------------------------------------------------------
	// ���� ��ǥ�� �ִ� ������Ʈ ã��
	//--------------------------------------------------------------------
	std::list<BaseObject*>* tile;
	for (int i = 0; i < 4; i++)
	{
		tile = _server->GetTile(dx[i], dy[i]);
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
