#include "stdafx.h"
#include "BaseObject.h"
#include "ObjectManager.h"

BaseObject::BaseObject(WORD type) : _type(type)
{
	_objectID = ObjectManager::GetInstance()->NextObjectID();
}
BaseObject::~BaseObject()
{
}
INT64 BaseObject::GetID()
{
	return _objectID;
}
WORD BaseObject::GetType()
{
	return _type;
}
float BaseObject::GetPosX()
{
	return _posX;
}
float BaseObject::GetPosY()
{
	return _posY;
}
int BaseObject::GetTileX()
{
	return _tileX;
}
int BaseObject::GetTileY()
{
	return _tileY;
}
int BaseObject::GetOldSectorX()
{
	return _oldSector.x;
}
int BaseObject::GetOldSectorY()
{
	return _oldSector.y;
}
int BaseObject::GetCurSectorX()
{
	return _curSector.x;
}
int BaseObject::GetCurSectorY()
{
	return _curSector.y;
}
