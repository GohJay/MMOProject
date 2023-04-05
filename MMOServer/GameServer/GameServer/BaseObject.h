#pragma once

typedef __int64 OBJECT_ID;
class GameServer;

enum OBJECT_TYPE
{
	PLAYER = 0,
	MONSTER,
	CRISTAL,
};

struct SECTOR
{
	int x;
	int y;
};

struct SECTOR_AROUND
{
	int count;
	SECTOR around[9];
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
	INT64 _objectID;
	WORD _type;
	float _posX;
	float _posY;
	int _tileX;
	int _tileY;
	SECTOR _oldSector;
	SECTOR _curSector;
};
