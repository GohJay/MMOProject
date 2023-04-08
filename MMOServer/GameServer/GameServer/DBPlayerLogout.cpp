#include "stdafx.h"
#include "DBPlayerLogout.h"
#include "GameData.h"

DBPlayerLogout::DBPlayerLogout(Jay::DBConnector* db, INT64 accountno, float posX, float posY, int tileX, int tileY, int rotation, int hp, INT64 exp) :
	_db(db), _accountno(accountno), _posX(posX), _posY(posY), _tileX(tileX), _tileY(tileY), _rotation(rotation), _hp(hp), _exp(exp)
{
}
DBPlayerLogout::~DBPlayerLogout()
{
}
void DBPlayerLogout::Exec()
{
	_db->Execute(L"START TRANSACTION;");

	UpdateAccountStatus();
	UpdateCharacter();
	SelectCristal();
	InsertLog();

	_db->Execute(L"COMMIT;");
}
void DBPlayerLogout::UpdateAccountStatus()
{
	_db->ExecuteUpdate(L"UPDATE accountdb.status SET status = %d WHERE accountno = %lld;"
		, ACCOUNT_STATUS_LOGOUT
		, _accountno);
}
void DBPlayerLogout::UpdateCharacter()
{
	_db->ExecuteUpdate(L"UPDATE gamedb.character SET posx = %f, posy = %f, tilex = %d, tiley = %d, rotation = %d, hp = %d, exp = %lld WHERE accountno = %lld;"
		, _posX
		, _posY
		, _tileX
		, _tileY
		, _rotation
		, _hp
		, _exp
		, _accountno);
}
void DBPlayerLogout::SelectCristal()
{
	sql::ResultSet* res;
	res = _db->ExecuteQuery(L"SELECT cristal FROM gamedb.character WHERE accountno = %lld;", _accountno);
	if (res->next())
		_totalCristal = res->getInt(1);
}
void DBPlayerLogout::InsertLog()
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
void DBPlayerLogout::InsertLogData()
{
	tm stTime;
	time_t timer;
	timer = time(NULL);
	localtime_s(&stTime, &timer);

	_db->ExecuteUpdate(L"INSERT INTO logdb.gamelog_%d%02d(accountno, type, param1, param2, param3, param4) VALUES(%lld, %d, %d, %d, %d, %d);"
		, stTime.tm_year + 1900
		, stTime.tm_mon + 1
		, _accountno
		, GAMEDB_LOG_TYPE_LOGOUT
		, _tileX
		, _tileY
		, _totalCristal
		, _hp);
}
void DBPlayerLogout::CreateLogTable()
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
