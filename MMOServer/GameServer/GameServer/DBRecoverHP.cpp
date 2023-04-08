#include "stdafx.h"
#include "DBRecoverHP.h"
#include "GameData.h"

DBRecoverHP::DBRecoverHP(Jay::DBConnector* db, INT64 accountno, int oldHP, int newHP, int sitTimeSec) :
	_db(db), _accountno(accountno), _oldHP(oldHP), _newHP(newHP), _sitTimeSec(sitTimeSec)
{
}
DBRecoverHP::~DBRecoverHP()
{
}
void DBRecoverHP::Exec()
{
	InsertLog();
}
void DBRecoverHP::InsertLog()
{
	try
	{
		InsertLogData();
	}
	catch (sql::SQLException& ex)
	{
		// Table doesn't exist
		if (ex.getErrorCode() == 1146)
		{
			CreateLogTable();
			InsertLogData();
			return;
		}

		throw ex;
	}
}
void DBRecoverHP::InsertLogData()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	_db->ExecuteUpdate(L"INSERT INTO logdb.gamelog_%d%02d(accountno, type, param1, param2, param3) VALUES(%lld, %d, %d, %d, %d);"
		, stTime.tm_year + 1900
		, stTime.tm_mon + 1
		, _accountno
		, GAMEDB_LOG_TYPE_RECOVER_HP
		, _oldHP
		, _newHP
		, _sitTimeSec);
}
void DBRecoverHP::CreateLogTable()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	try
	{
		_db->Execute(L"CREATE TABLE logdb.gamelog_%d%02d LIKE gamelog_template;"
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
