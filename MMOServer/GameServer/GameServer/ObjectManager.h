#pragma once
#include "BaseObject.h"
#include <list>
#include <atomic>

class ObjectManager
{
private:
	ObjectManager();
	~ObjectManager();
public:
	static ObjectManager* GetInstance();
	void Update();
	void Cleanup();
	INT64 NextObjectID();
	void InsertObject(BaseObject* object);
private:
	std::atomic<__int64> _objectKey;
	std::list<BaseObject*> _objectList;
	static ObjectManager _instance;
};
