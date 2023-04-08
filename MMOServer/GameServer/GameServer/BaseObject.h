#pragma once
#include "Sector.h"

typedef __int64 OBJECT_ID;
class GameContent;
class PlayerObject;
class MonsterObject;
class CristalObject;

enum OBJECT_TYPE
{
	PLAYER = 0,
	MONSTER,
	CRISTAL,
};

class BaseObject
{
public:
	BaseObject(WORD type);
	virtual ~BaseObject();
public:
	virtual bool Update() = 0;
	virtual void Dispose() = 0;
public:
	void DeleteObject();
	bool IsDeleted();
	INT64 GetID();
	WORD GetType();
	float GetPosX();
	float GetPosY();
	int GetTileX();
	int GetTileY();
	int GetOldSectorX();
	int GetOldSectorY();
	int GetCurSectorX();
	int GetCurSectorY();
protected:
	bool _deleted;
	INT64 _objectID;
	WORD _type;
	float _posX;
	float _posY;
	int _tileX;
	int _tileY;
	SECTOR _oldSector;
	SECTOR _curSector;
};
