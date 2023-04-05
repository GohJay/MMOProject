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
	static MonsterObject* Alloc(GameServer* server);
	static void Free(MonsterObject* monster);
public:
	virtual bool Update() override;
	virtual void Dispose() override;
	void Initial(BYTE type, WORD spawnZone);
	void Release();
	BYTE GetMonsterType();
	USHORT GetRotation();
	int GetHP();
	bool IsDie();
	void Respawn();
	bool UpdateTile();
	bool UpdateSector();
	void Move(float targetX, float targetY);
	void Damage(int damage);
private:
	bool _release;
	BYTE _monsterType;
	WORD _spawnZone;
	USHORT _rotation;
	DWORD _lastDeadTime;
	int _hp;
	bool _die;
	int _power;
	float _radian;
	float _targetX;
	float _targetY;
	GameServer* _server;
	static Jay::LFObjectPool_TLS<MonsterObject> _pool;
};
