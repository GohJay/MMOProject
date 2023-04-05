#pragma once
#include "IDBJob.h"
#include "../../Common/DBConnector.h"

class DBPlayerLogin : public IDBJob
{
public:
	DBPlayerLogin(Jay::DBConnector* db, INT64 accountno, int tileX, int tileY, int cristal, int hp);
	~DBPlayerLogin();
public:
	virtual void Exec() override;
private:
	void UpdateAccountStatus();
	void InsertLog();
	void InsertLogData();
	void CreateLogTable();
private:
	Jay::DBConnector* _db;
	INT64 _accountno;
	int _tileX;
	int _tileY;
	int _cristal;
	int _hp;
};
