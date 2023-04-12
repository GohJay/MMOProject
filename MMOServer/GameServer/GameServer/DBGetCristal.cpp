#include "stdafx.h"
#include "DBGetCristal.h"
#include "GameData.h"

DBGetCristal::DBGetCristal(Jay::DBConnector* db, INT64 accountno, int getCristal) :
	_db(db), _accountno(accountno), _getCristal(getCristal)
{
}
DBGetCristal::~DBGetCristal()
{
}
void DBGetCristal::Exec()
{
	_db->Execute(L"START TRANSACTION;");

	UpdateCharacter();
	InsertLog();

	_db->Execute(L"COMMIT;");
}
void DBGetCristal::UpdateCharacter()
{
	sql::ResultSet* res;
	res = _db->ExecuteQuery(L"SELECT cristal FROM gamedb.character WHERE accountno = %lld FOR UPDATE;", _accountno);
	if (res->next())
	{
		_totalCristal = res->getInt(1);
		_totalCristal += _getCristal;
	}
	_db->ClearQuery(res);

	_db->ExecuteUpdate(L"UPDATE gamedb.character SET cristal = %d WHERE accountno = %lld;", _totalCristal, _accountno);
}
void DBGetCristal::InsertLog()
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
void DBGetCristal::InsertLogData()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	_db->ExecuteUpdate(L"INSERT INTO logdb.gamelog_%d%02d(accountno, type, param1, param2) VALUES(%lld, %d, %d, %d);"
		, stTime.tm_year + 1900
		, stTime.tm_mon + 1
		, _accountno
		, GAMEDB_LOG_TYPE_PICK_CRISTAL
		, _getCristal
		, _totalCristal);
}
void DBGetCristal::CreateLogTable()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	try
	{
		_db->Execute(L"CREATE TABLE logdb.gamelog_%d%02d LIKE logdb.gamelog_template;"
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
