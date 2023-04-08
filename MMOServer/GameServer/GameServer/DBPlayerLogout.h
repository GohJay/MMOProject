#pragma once
#include "IDBJob.h"
#include "../../Common/DBConnector.h"

class DBPlayerLogout : public IDBJob
{
public:
	DBPlayerLogout(Jay::DBConnector* db, INT64 accountno, float posX, float posY, int tileX, int tileY, int rotation, int hp, INT64 exp);
	~DBPlayerLogout();
public:
	virtual void Exec() override;
private:
	void UpdateAccountStatus();
	void UpdateCharacter();
	void SelectCristal();
	void InsertLog();
	void InsertLogData();
	void CreateLogTable();
private:
	Jay::DBConnector* _db;
	INT64 _accountno;
	float _posX;
	float _posY;
	int _tileX;
	int _tileY;
	int _rotation;
	int _hp;
	INT64 _exp;
	int _die;
	int _totalCristal;
};
