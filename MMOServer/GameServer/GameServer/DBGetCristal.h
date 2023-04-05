#pragma once
#include "IDBJob.h"
#include "../../Common/DBConnector.h"

class DBGetCristal : public IDBJob
{
public:
	DBGetCristal(Jay::DBConnector* db, INT64 accountno, int getCristal);
	~DBGetCristal();
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
	int _getCristal;
	int _totalCristal;
};
