#ifndef __DB_CONNECTOR_TLS__H_
#define __DB_CONNECTOR_TLS__H_
#include "DBConnector.h"
#include "LockFreeStack.h"

namespace Jay
{
	class DBConnector_TLS
	{
		/**
		* @file		DBConnector.h
		* @brief	DB Connector Class TLS ����
		* @details	��Ƽ������ ȯ�濡���� DB ����� ���� Ŀ���� Ŭ����
		* @author   ������
		* @date		2023-02-11
		* @version  1.0.0
		**/
	private:
		struct PROPERTY
		{
			wchar_t ipaddress[16];
			int port;
			wchar_t user[32];
			wchar_t passwd[32];
			wchar_t schema[32];
			bool opt_reconnect;
		};
	public:
		DBConnector_TLS();
		~DBConnector_TLS();
	public:
		/**
		* @brief	DB ���� ���� ����
		* @details
		* @param	const wchar_t*(�ּ�), int(��Ʈ), const wchar_t*(����), const wchar_t*(�н�����), const wchar_t*(��Ű��), bool(�翬�� ����)
		* @return	void
		**/
		void SetProperty(const wchar_t* ipaddress, int port, const wchar_t* user, const wchar_t* passwd, const wchar_t* schema, bool reconnect = true);

		/**
		* @brief	DDL, DCL, TCL ���� ����
		* @details	������ ���Ǿ�, ������ �����, Ʈ������ ����� ���� ����
		* @param	const wchar_t*(����)
		* @return	void
		**/
		void Execute(const wchar_t* query, ...);

		/**
		* @brief	DML �� �����Ϳ� ������ ���ϴ� ���� ����
		* @details	INSERT, UPDATE, DELETE ���� ����
		* @param	const wchar_t*(����)
		* @return	void
		**/
		void ExecuteUpdate(const wchar_t* query, ...);

		/**
		* @brief	DML �� �����͸� ��ȸ�ϴ� ���� ����
		* @details	SELECT ���� ����
		* @param	const wchar_t*(����)
		* @return	sql::ResultSet*(��ȸ�� ������)
		**/
		sql::ResultSet* ExecuteQuery(const wchar_t* query, ...);

		/**
		* @brief	��ȸ�� ������ �ݳ�
		* @details	SELECT ������ ���� ��ȸ�� ������ �ݳ�
		* @param	sql::ResultSet*(��ȸ�� ������)
		* @return	void
		**/
		void ClearQuery(sql::ResultSet* res);
	private:
		DBConnector* GetCurrentDB();
	private:
		PROPERTY _property;
		DWORD _tlsDB;
		LockFreeStack<DBConnector*> _gcStack;
	};
}

#endif
