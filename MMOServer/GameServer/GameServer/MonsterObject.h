#pragma once
#include "BaseObject.h"
#include "../../Common/LFObjectPool_TLS.h"

enum SPAWN_ZONE
{
	ZONE_1 = 0,
	ZONE_2,
	ZONE_3,
	ZONE_4,
	ZONE_5,
	ZONE_6,
	ZONE_7,
};

class MonsterObject : public BaseObject
{
public:
	MonsterObject();
	virtual ~MonsterObject() override;
public:
	static MonsterObject* Alloc(GameContent* game);
	static void Free(MonsterObject* monster);
public:
	virtual bool Update() override;
	virtual void Dispose() override;
	void Init(BYTE type, WORD spawnZone);
	BYTE GetMonsterType();
	USHORT GetRotation();
	int GetHP();
	bool IsDie();
	bool UpdateTile();
	bool UpdateSector();
	void Respawn();
	void Move();
	void Attack();
	void Damage(PlayerObject* attackPlayer, int damage);
private:
	bool IsInMonsterZone(int tileX, int tileY);
	bool IsSightInRange(int tileX, int tileY);
	bool IsAttackInRange();
	bool IsTargetVisibleAndInRange();
private:
	BYTE _monsterType;
	WORD _spawnZone;
	USHORT _rotation;
	bool _die;
	int _power;
	int _hp;
	int _maxHP;
	float _scala;
	DWORD _lastDeadTime;
	DWORD _lastMoveTime;
	DWORD _lastAttackTime;
	INT64 _targetPlayerID;
	PlayerObject* _targetPlayer;
	GameContent* _game;
	static Jay::LFObjectPool_TLS<MonsterObject> _pool;
};
