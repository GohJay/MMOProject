#pragma once
#include "BaseObject.h"
#include "../../Common/LFObjectPool_TLS.h"

class CristalObject : public BaseObject
{
public:
	CristalObject();
	virtual ~CristalObject() override;
public:
	static CristalObject* Alloc(GameContent* game);
	static void Free(CristalObject* cristal);
public:
	virtual bool Update() override;
	virtual void Dispose() override;
	void Init(BYTE type, float posX, float posY);
	BYTE GetCristalType();
	int GetAmount();
private:
	bool CheckExpirationTime();
private:
	BYTE _cristalType;
	DWORD _creationTime;
	int _amount;
	GameContent* _game;
	static Jay::LFObjectPool_TLS<CristalObject> _pool;
};
