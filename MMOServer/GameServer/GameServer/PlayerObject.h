#pragma once
#include "BaseObject.h"
#include "../../Common/LFObjectPool_TLS.h"

enum CHARACTER_TYPE
{
	GOLEM = 1,
	KNIGHT,
	ORC,
	ELF,
	ARCHER,
};

class PlayerObject : public BaseObject
{
public:
	PlayerObject();
	virtual ~PlayerObject() override;
public:
	static PlayerObject* Alloc();
	static void Free(PlayerObject* player);
public:
	virtual bool Update() override;
	virtual void Dispose() override;
	void Initial(WCHAR* nickname, BYTE type, float posX, float posY, USHORT rotation, int cristal, int hp, INT64 exp, USHORT level, bool die);
	void Release();
	void SetGameServer(GameServer* server);
	INT64 GetAccountNo();
	INT64 GetSessionID();
	BYTE GetCharacterType();
	WCHAR* GetNickname();
	USHORT GetRotation();
	int GetCristal();
	int GetHP();
	INT64 GetExp();
	USHORT GetLevel();
	BYTE GetVKey();
	BYTE GetHKey();
	int GetPower();
	bool IsDie();
	bool IsSit();
	bool IsLogin();
	void Auth(INT64 sessionID, INT64 accountNo);
	bool UpdateTile();
	bool UpdateSector();
	void Restart();
	void MoveStart(float posX, float posY, USHORT rotation, BYTE vKey, BYTE hKey);
	void MoveStop(float posX, float posY, USHORT rotation);
	void Attack1();
	void Attack2();
	void Pick();
	void Sit();
	void Damage(int damage);
private:
	bool FindForwardDirectionObject(BaseObject** object, OBJECT_TYPE type);
private:
	bool _release;
	INT64 _sessionID;
	INT64 _accountNo;
	BYTE _characterType;
	WCHAR _nickname[20];
	USHORT _rotation;
	int _cristal;
	int _hp;
	INT64 _exp;
	USHORT _level;
	bool _die;
	bool _sit;
	BYTE _vKey;
	BYTE _hKey;
	int _power;
	bool _login;
	GameServer* _server;
	static Jay::LFObjectPool_TLS<PlayerObject> _pool;
};
