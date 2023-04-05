#pragma once
#include "BaseObject.h"
#include "../../Common/LFObjectPool_TLS.h"

class CristalObject : public BaseObject
{
public:
	CristalObject();
	virtual ~CristalObject() override;
public:
	static CristalObject* Alloc(GameServer* server);
	static void Free(CristalObject* cristal);
public:
	virtual bool Update() override;
	virtual void Dispose() override;
	void Initial(BYTE type, float posX, float posY);
	void Release();
	BYTE GetCristalType();
private:
	bool CheckExpirationTime();
private:
	bool _release;
	BYTE _cristalType;
	DWORD _creationTime;
	GameServer* _server;
	static Jay::LFObjectPool_TLS<CristalObject> _pool;
};
