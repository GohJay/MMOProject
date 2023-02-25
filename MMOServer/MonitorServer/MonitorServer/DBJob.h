#pragma once
#include "../../Common/DBConnector.h"

class IDBJob
{
public:
	virtual void Exec(Jay::DBConnector* db) = 0;
};

class DBMonitoringLog : public IDBJob
{
public:
	DBMonitoringLog(int serverno, int type, int avr, int min, int max);
public:
	void Exec(Jay::DBConnector* db) override;
private:
	void InsertLogData(Jay::DBConnector* db);
	void CreateLogTable(Jay::DBConnector* db);
private:
	int _serverno;
	int _type;
	int _avr;
	int _min;
	int _max;
};
