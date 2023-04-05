#include "stdafx.h"
#include "DBPlayerRestart.h"
#include "GameData.h"

DBPlayerRestart::DBPlayerRestart(Jay::DBConnector* db, INT64 accountno, int tileX, int tileY) :
	_db(db), _accountno(accountno), _tileX(tileX), _tileY(tileY)
{
}
DBPlayerRestart::~DBPlayerRestart()
{
}
void DBPlayerRestart::Exec()
{
	InsertLog();
}
void DBPlayerRestart::InsertLog()
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
void DBPlayerRestart::InsertLogData()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	_db->ExecuteUpdate(L"INSERT INTO logdb.gamelog_%d%02d(accountno, type, param1, param2) VALUES(%lld, %d, %d, %d);"
		, stTime.tm_year + 1900
		, stTime.tm_mon + 1
		, _accountno
		, GAMEDB_LOG_TYPE_PLAYER_RESTART
		, _tileX
		, _tileY);
}
void DBPlayerRestart::CreateLogTable()
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
