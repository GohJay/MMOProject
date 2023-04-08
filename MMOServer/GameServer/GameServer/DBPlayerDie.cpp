#include "stdafx.h"
#include "DBPlayerDie.h"
#include "GameData.h"

DBPlayerDie::DBPlayerDie(Jay::DBConnector* db, INT64 accountno, int tileX, int tileY, int minusCristal) :
	_db(db), _accountno(accountno), _tileX(tileX), _tileY(tileY), _minusCristal(minusCristal)
{
}
DBPlayerDie::~DBPlayerDie()
{
}
void DBPlayerDie::Exec()
{
	_db->Execute(L"START TRANSACTION;");

	UpdateCharacter();
	InsertLog();

	_db->Execute(L"COMMIT;");
}
void DBPlayerDie::UpdateCharacter()
{
	sql::ResultSet* res;
	res = _db->ExecuteQuery(L"SELECT cristal FROM gamedb.character WHERE accountno = %lld FOR UPDATE;", _accountno);
	if (res->next())
	{
		_totalCristal = res->getInt(1);
		_totalCristal -= _minusCristal;
	}

	_db->ExecuteUpdate(L"UPDATE gamedb.character SET cristal = %d, die = 1 WHERE accountno = %lld;", _totalCristal, _accountno);
}
void DBPlayerDie::InsertLog()
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
void DBPlayerDie::InsertLogData()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	_db->ExecuteUpdate(L"INSERT INTO logdb.gamelog_%d%02d(accountno, type, param1, param2, param3, param4) VALUES(%lld, %d, %d, %d, %d, %d);"
		, stTime.tm_year + 1900
		, stTime.tm_mon + 1
		, _accountno
		, GAMEDB_LOG_TYPE_PLAYER_DIE
		, _tileX
		, _tileY
		, _minusCristal
		, _totalCristal);
}
void DBPlayerDie::CreateLogTable()
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
