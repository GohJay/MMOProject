#include "stdafx.h"
#include "ObjectManager.h"

ObjectManager ObjectManager::_instance;

ObjectManager::ObjectManager() : _objectKey(0)
{
}
ObjectManager::~ObjectManager()
{
}
ObjectManager* ObjectManager::GetInstance()
{
	return &_instance;
}
void ObjectManager::Update()
{
	for (auto iter = _objectList.begin(); iter != _objectList.end();)
	{
		BaseObject* object = *iter;
		if (!object->Update())
		{
			object->Dispose();
			iter = _objectList.erase(iter);
			continue;
		}
		++iter;
	}
}
void ObjectManager::Cleanup()
{
	for (auto iter = _objectList.begin(); iter != _objectList.end();)
	{
		BaseObject* object = *iter;
		object->Dispose();
		iter = _objectList.erase(iter);
	}
}
INT64 ObjectManager::NextObjectID()
{
	return ++_objectKey;
}
void ObjectManager::InsertObject(BaseObject* object)
{
	_objectList.push_back(object);
}
