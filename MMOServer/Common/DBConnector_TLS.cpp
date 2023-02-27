#include "DBConnector_TLS.h"
#include "StringUtil.h"
#include <strsafe.h>

using namespace Jay;

__declspec(thread) DBConnector* DBConnector_TLS::_tlsDB = NULL;
DBConnector_TLS::DBConnector_TLS()
{
}
DBConnector_TLS::~DBConnector_TLS()
{
	DBConnector* db;
	while (!_gcStack.empty())
	{
		_gcStack.Pop(db);
		db->Disconnect();
		delete db;
	}
}
void DBConnector_TLS::SetProperty(const wchar_t* ipaddress, int port, const wchar_t* user, const wchar_t* passwd, const wchar_t* schema, bool reconnect)
{
	StringCchCopy(_property.ipaddress, sizeof(_property.ipaddress) / 2, ipaddress);
	_property.port = port;
	StringCchCopy(_property.user, sizeof(_property.user) / 2, user);
	StringCchCopy(_property.passwd, sizeof(_property.passwd) / 2, passwd);
	StringCchCopy(_property.schema, sizeof(_property.schema) / 2, schema);
	_property.opt_reconnect = reconnect;
}
void DBConnector_TLS::Execute(const wchar_t* query, ...)
{
	WCHAR queryw[MAX_QUERYLEN];
	va_list args;
	va_start(args, query);
	StringCchVPrintf(queryw, MAX_QUERYLEN, query, args);
	va_end(args);

	DBConnector* db = GetCurrentDB();
	db->Execute(queryw);
}
void DBConnector_TLS::ExecuteUpdate(const wchar_t* query, ...)
{
	WCHAR queryw[MAX_QUERYLEN];
	va_list args;
	va_start(args, query);
	StringCchVPrintf(queryw, MAX_QUERYLEN, query, args);
	va_end(args);

	DBConnector* db = GetCurrentDB();
	db->ExecuteUpdate(queryw);
}
sql::ResultSet* DBConnector_TLS::ExecuteQuery(const wchar_t* query, ...)
{
	WCHAR queryw[MAX_QUERYLEN];
	va_list args;
	va_start(args, query);
	StringCchVPrintf(queryw, MAX_QUERYLEN, query, args);
	va_end(args);

	DBConnector* db = GetCurrentDB();
	return db->ExecuteQuery(queryw);
}
void DBConnector_TLS::ClearQuery(sql::ResultSet* res)
{
	DBConnector* db = GetCurrentDB();
	db->ClearQuery(res);
}
DBConnector* DBConnector_TLS::GetCurrentDB()
{
	DBConnector* db = _tlsDB;
	if (db == NULL)
	{
		db = new DBConnector();
		db->Connect(_property.ipaddress
			, _property.port
			, _property.user
			, _property.passwd
			, _property.schema
			, _property.opt_reconnect);

		_gcStack.Push(db);
		_tlsDB = db;
	}
	return db;
}
