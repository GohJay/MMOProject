#ifndef __DB_CONNECTOR__H_
#define __DB_CONNECTOR__H_
#define STATIC_CONCPP
#include "../Lib/MySQL/include/mysql/jdbc.h"
#include "Lock.h"
#include <Windows.h>
#include <thread>

#define MAX_QUERYLEN		512
#define SLOWQUERY_TIME		200
#define PROFILE_TERM		1000 * 60 * 10

namespace Jay
{
	class DBConnector
	{
		/**
		* @file		DBConnector.h
		* @brief	DB Connector Class
		* @details	DB 사용을 위한 커넥터 클래스
		* @author   고재현
		* @date		2023-02-07
		* @version  1.0.0
		**/
	private:
		struct PROFILE
		{
			DWORD64 iTotalTime;         // 전체 사용시간 카운터 Time. (출력시 호출회수로 나누어 평균 구함)
			DWORD64 iMin;	            // 최소 사용시간 카운터 Time. (초단위로 계산하여 저장)
			DWORD64 iMax;	            // 최대 사용시간 카운터 Time. (초단위로 계산하여 저장)
			DWORD64 iCall;              // 누적 호출 횟수.
			LARGE_INTEGER lStartTime;   // 프로파일 실행 시간.
		};
	public:
		DBConnector();
		~DBConnector();
	public:
		/**
		* @brief	DB 연결
		* @details
		* @param	const wchar_t*(주소), int(포트), const wchar_t*(유저), const wchar_t*(패스워드), const wchar_t*(스키마), bool(재연결 여부)
		* @return	void
		**/
		void Connect(const wchar_t* ipaddress, int port, const wchar_t* user, const wchar_t* passwd, const wchar_t* schema, bool reconnect = true);

		/**
		* @brief	DB 연결 종료
		* @details
		* @param	void
		* @return	void
		**/
		void Disconnect(void);

		/**
		* @brief	DDL, DCL, TCL 쿼리 실행
		* @details	데이터 정의어, 데이터 제어어, 트렌젝션 제어어 쿼리 실행
		* @param	const wchar_t*(쿼리)
		* @return	void
		**/
		void Execute(const wchar_t* query, ...);

		/**
		* @brief	DML 중 데이터에 변경을 가하는 쿼리 실행
		* @details	INSERT, UPDATE, DELETE 쿼리 실행
		* @param	const wchar_t*(쿼리)
		* @return	void
		**/
		void ExecuteUpdate(const wchar_t* query, ...);

		/**
		* @brief	DML 중 데이터를 조회하는 쿼리 실행
		* @details	SELECT 쿼리 실행
		* @param	const wchar_t*(쿼리)
		* @return	sql::ResultSet*(조회한 데이터)
		**/
		sql::ResultSet* ExecuteQuery(const wchar_t* query, ...);

		/**
		* @brief	조회한 데이터 반납
		* @details	SELECT 쿼리를 통해 조회한 데이터 반납
		* @param	sql::ResultSet*(조회한 데이터)
		* @return	void
		**/
		void ClearQuery(sql::ResultSet* res);
	private:
		void ProfileBegin(void);
		void ProfileEnd(void);
		void ProfileDataOutText(void);
	private:
		wchar_t _queryw[MAX_QUERYLEN];
		char _query[MAX_QUERYLEN];
		PROFILE _profile;
		DWORD _lastProfileTime;
		LARGE_INTEGER _freq;
		sql::Connection* _conn;
		sql::Statement* _stmt;
		static sql::Driver* _driver;
	};
}

#endif
