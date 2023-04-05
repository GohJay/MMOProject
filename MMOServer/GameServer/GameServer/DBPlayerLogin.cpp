#include "stdafx.h"
#include "DBPlayerLogin.h"
#include "GameData.h"

DBPlayerLogin::DBPlayerLogin(Jay::DBConnector* db, INT64 accountno, int tileX, int tileY, int cristal, int hp) :
	_db(db), _accountno(accountno), _tileX(tileX), _tileY(tileY), _cristal(cristal), _hp(hp)
{
}
DBPlayerLogin::~DBPlayerLogin()
{
}
void DBPlayerLogin::Exec()
{
	_db->Execute(L"START TRANSACTION;");

	UpdateAccountStatus();
	InsertLog();

	_db->Execute(L"COMMIT;");
}
void DBPlayerLogin::UpdateAccountStatus()
{
	_db->ExecuteUpdate(L"UPDATE accountdb.status SET status = %d WHERE accountno = %lld;"
		, ACCOUNT_STATUS_GAME_PLAY
		, _accountno);
}
void DBPlayerLogin::InsertLog()
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
void DBPlayerLogin::InsertLogData()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	_db->ExecuteUpdate(L"INSERT INTO logdb.gamelog_%d%02d(accountno, type, param1, param2, param3, param4) VALUES(%lld, %d, %d, %d, %d, %d);"
		, stTime.tm_year + 1900
		, stTime.tm_mon + 1
		, _accountno
		, GAMEDB_LOG_TYPE_LOGIN
		, _tileX
		, _tileY
		, _cristal
		, _hp);
}
void DBPlayerLogin::CreateLogTable()
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
