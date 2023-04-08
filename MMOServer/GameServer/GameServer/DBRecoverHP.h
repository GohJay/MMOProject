#pragma once
#include "IDBJob.h"
#include "../../Common/DBConnector.h"

class DBRecoverHP : public IDBJob
{
public:
	DBRecoverHP(Jay::DBConnector* db, INT64 accountno, int oldHP, int newHP, int sitTimeSec);
	~DBRecoverHP();
public:
	virtual void Exec() override;
private:
	void InsertLog();
	void InsertLogData();
	void CreateLogTable();
private:
	Jay::DBConnector* _db;
	INT64 _accountno;
	int _oldHP;
	int _newHP;
	int _sitTimeSec;
};
