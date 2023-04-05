#pragma once
#include "IDBJob.h"
#include "../../Common/DBConnector.h"

class DBPlayerDie : public IDBJob
{
public:
	DBPlayerDie(Jay::DBConnector* db, INT64 accountno, int tileX, int tileY, int minusCristal);
	~DBPlayerDie();
public:
	virtual void Exec() override;
private:
	void UpdateCharacter();
	void InsertLog();
	void InsertLogData();
	void CreateLogTable();
private:
	Jay::DBConnector* _db;
	INT64 _accountno;
	int _tileX;
	int _tileY;
	int _minusCristal;
	int _totalCristal;
};
