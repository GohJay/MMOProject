#include "stdafx.h"
#include "CristalObject.h"
#include "ObjectManager.h"
#include "GameContent.h"
#include "Packet.h"
#include "../../Lib/Network/include/NetPacket.h"

using namespace Jay;

LFObjectPool<CristalObject> CristalObject::_pool(0, true);

CristalObject::CristalObject() : BaseObject(CRISTAL)
{
}
CristalObject::~CristalObject()
{
}
CristalObject* CristalObject::Alloc(GameContent* game)
{
	CristalObject* cristal = _pool.Alloc();
	cristal->_game = game;
	return cristal;
}
void CristalObject::Free(CristalObject* cristal)
{
	_pool.Free(cristal);
}
bool CristalObject::Update()
{
	if (IsDeleted())
		return false;

	if (CheckExpirationTime())
		return false;

	return true;
}
void CristalObject::Dispose()
{
	CristalObject::Free(this);
}
void CristalObject::Init(BYTE type, float posX, float posY)
{
	_cristalType = type;
	_posX = posX;
	_posY = posY;
	_tileX = _posX * 2;
	_tileY = _posY * 2;
	_curSector.x = _tileX / dfSECTOR_SIZE_X;
	_curSector.y = _tileY / dfSECTOR_SIZE_Y;
	_creationTime = timeGetTime();

	DATA_CRISTAL* dataCristal = _game->GetDataCristal(_cristalType);
	_amount = dataCristal->amount;
}
BYTE CristalObject::GetCristalType()
{
	return _cristalType;
}
int CristalObject::GetAmount()
{
	return _amount;
}
bool CristalObject::CheckExpirationTime()
{
	//--------------------------------------------------------------------
	// ũ����Ż�� ��ȿ �ð��� Ȯ���Ͽ� ���� ó��
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - _creationTime > dfCRISTAL_TIME_LIMIT)
	{
		_game->DeleteCristal(this);
		return true;
	}
	return false;
}
