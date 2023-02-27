#include "stdafx.h"
#include "DBJob.h"

DBMonitoringLog::DBMonitoringLog(int serverno, int type, int avr, int min, int max) 
	: _serverno(serverno), _type(type), _avr(avr), _min(min), _max(max)
{
}
DBMonitoringLog::~DBMonitoringLog()
{
}
void DBMonitoringLog::Exec(Jay::DBConnector* db)
{
	try
	{
		InsertLogData(db);
	}
	catch (sql::SQLException& ex)
	{
		// Table doesn't exist
		if (ex.getErrorCode() == 1146)	
		{
			CreateLogTable(db);
			InsertLogData(db);
			return;
		}

		throw ex;
	}
}
void DBMonitoringLog::InsertLogData(Jay::DBConnector* db)
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	db->ExecuteUpdate(L"INSERT INTO monitorlog_%d%02d(serverno, type, avr, min, max) VALUES(%d, %d, %d, %d, %d);"
		, stTime.tm_year + 1900
		, stTime.tm_mon + 1
		, _serverno
		, _type
		, _avr
		, _min
		, _max);
}
void DBMonitoringLog::CreateLogTable(Jay::DBConnector* db)
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	try
	{
		db->Execute(L"CREATE TABLE monitorlog_%d%02d LIKE monitorlog_template;"
			, stTime.tm_year + 1900
			, stTime.tm_mon + 1);
	}
	catch (sql::SQLException& ex)
	{
		// Table already exists
		if (ex.getErrorCode() == 1050)	
			return;

		throw ex;
	}
}
