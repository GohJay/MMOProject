#include "stdafx.h"
#include "CristalObject.h"
#include "ObjectManager.h"
#include "GameServer.h"
#include "Packet.h"
#include "../../Lib/Network/include/NetPacket.h"

using namespace Jay;

LFObjectPool_TLS<CristalObject> CristalObject::_pool(0, true);

CristalObject::CristalObject() : BaseObject(CRISTAL), _release(false)
{
}
CristalObject::~CristalObject()
{
}
CristalObject* CristalObject::Alloc(GameServer* server)
{
	CristalObject* cristal = _pool.Alloc();
	cristal->_server = server;
	return cristal;
}
void CristalObject::Free(CristalObject* cristal)
{
	_pool.Free(cristal);
}
bool CristalObject::Update()
{
	if (_release)
		return false;

	if (CheckExpirationTime())
		return false;

	return true;
}
void CristalObject::Dispose()
{
	CristalObject::Free(this);
}
void CristalObject::Initial(BYTE type, float posX, float posY)
{
	_cristalType = type;
	_posX = posX;
	_posY = posY;
	_tileX = _posX * 2;
	_tileY = _posY * 2;
	_curSector.x = _tileX / dfSECTOR_SIZE_X;
	_curSector.y = _tileY / dfSECTOR_SIZE_Y;
	_creationTime = timeGetTime();
}
void CristalObject::Release()
{
	_release = true;
}
BYTE CristalObject::GetCristalType()
{
	return _cristalType;
}
bool CristalObject::CheckExpirationTime()
{
	//--------------------------------------------------------------------
	// 크리스탈의 유효 시간을 확인하여 삭제 처리
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _creationTime > dfCRISTAL_TIME_LIMIT)
	{
		_server->DeleteCristal(this);
		return true;
	}
	return false;
}
