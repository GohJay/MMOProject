#include "DBConnector.h"
#include "Logger.h"
#include "CrashDump.h"
#include "StringUtil.h"
#include <strsafe.h>
#include <memory>

using namespace Jay;

sql::Driver* DBConnector::_driver = get_driver_instance();
DBConnector::DBConnector()
{
    memset(&_profile, 0, sizeof(_profile));
    _lastProfileTime = timeGetTime();
    QueryPerformanceFrequency(&_freq);
}
DBConnector::~DBConnector()
{
}
void DBConnector::Connect(const wchar_t* ipaddress, int port, const wchar_t* user, const wchar_t* passwd, const wchar_t* schema, bool reconnect)
{
    sql::ConnectOptionsMap property;
    std::string hostName;
    std::string userName;
    std::string password;
    std::string database;
    UnicodeToString(ipaddress, hostName);
    UnicodeToString(user, userName);
    UnicodeToString(passwd, password);
    UnicodeToString(schema, database);

    try
    {
        //--------------------------------------------------------------------
        // DB 연결 정보 세팅
        //--------------------------------------------------------------------
        property["hostName"] = hostName.c_str();
        property["port"] = port;
        property["userName"] = userName.c_str();
        property["password"] = password.c_str();
        property["schema"] = database.c_str();
        property["OPT_RECONNECT"] = reconnect;

        //--------------------------------------------------------------------
        // DB 연결
        //--------------------------------------------------------------------
        _conn = _driver->connect(property);

        //--------------------------------------------------------------------
        // Statement 할당
        //--------------------------------------------------------------------
        _stmt = _conn->createStatement();
    }
    catch (sql::SQLException& ex)
    {
        std::wstring errMessage;
        std::wstring sqlState;
        MultiByteToWString(ex.what(), errMessage);
        MultiByteToWString(ex.getSQLStateCStr(), sqlState);

        Logger::WriteLog(L"DBConnector_Error"
            , LOG_LEVEL_ERROR
            , L"%s() - ErrorCode: %d, ErrorMessage: %s, SQLState: %s"
            , __FUNCTIONW__
            , ex.getErrorCode()
            , errMessage.c_str()
            , sqlState.c_str());

        throw ex;
    }
}
void DBConnector::Disconnect()
{
    delete _stmt;
    delete _conn;
}
void DBConnector::Execute(const wchar_t* query, ...)
{
    va_list args;
    va_start(args, query);
    StringCchVPrintf(_queryw, MAX_QUERYLEN, query, args);
    va_end(args);
    UnicodeToMultiByte(_queryw, _query);

    try
    {
        //--------------------------------------------------------------------
        // Statement 초기화
        //--------------------------------------------------------------------
        _stmt->clearAttributes();

        //--------------------------------------------------------------------
        // 쿼리 실행
        //--------------------------------------------------------------------
        ProfileBegin();
        _stmt->execute(_query);
        ProfileEnd();
    }
    catch (sql::SQLException& ex)
    {
        std::wstring errMessage;
        std::wstring sqlState;
        MultiByteToWString(ex.what(), errMessage);
        MultiByteToWString(ex.getSQLStateCStr(), sqlState);

        Logger::WriteLog(L"DBConnector_Error"
            , LOG_LEVEL_ERROR
            , L"%s() - Query: %s, ErrorCode: %d, ErrorMessage: %s, SQLState: %s"
            , __FUNCTIONW__
            , _queryw
            , ex.getErrorCode()
            , errMessage.c_str()
            , sqlState.c_str());

        throw ex;
    }
}
void DBConnector::ExecuteUpdate(const wchar_t* query, ...)
{
    va_list args;
    va_start(args, query);
    StringCchVPrintf(_queryw, MAX_QUERYLEN, query, args);
    va_end(args);
    UnicodeToMultiByte(_queryw, _query);

    try
    {
        //--------------------------------------------------------------------
        // Statement 초기화
        //--------------------------------------------------------------------
        _stmt->clearAttributes();

        //--------------------------------------------------------------------
        // 쿼리 실행
        //--------------------------------------------------------------------
        ProfileBegin();
        _stmt->executeUpdate(_query);
        ProfileEnd();
    }
    catch (sql::SQLException& ex)
    {
        std::wstring errMessage;
        std::wstring sqlState;
        MultiByteToWString(ex.what(), errMessage);
        MultiByteToWString(ex.getSQLStateCStr(), sqlState);

        Logger::WriteLog(L"DBConnector_Error"
            , LOG_LEVEL_ERROR
            , L"%s() - Query: %s, ErrorCode: %d, ErrorMessage: %s, SQLState: %s"
            , __FUNCTIONW__
            , _queryw
            , ex.getErrorCode()
            , errMessage.c_str()
            , sqlState.c_str());

        throw ex;
    }
}
sql::ResultSet* DBConnector::ExecuteQuery(const wchar_t* query, ...)
{
    va_list args;
    va_start(args, query);
    StringCchVPrintf(_queryw, MAX_QUERYLEN, query, args);
    va_end(args);
    UnicodeToMultiByte(_queryw, _query);

    sql::ResultSet* res;

    try
    {
        //--------------------------------------------------------------------
        // Statement 초기화
        //--------------------------------------------------------------------
        _stmt->clearAttributes();

        //--------------------------------------------------------------------
        // 쿼리 실행
        //--------------------------------------------------------------------
        ProfileBegin();
        res = _stmt->executeQuery(_query);
        ProfileEnd();
    }
    catch (sql::SQLException& ex)
    {
        std::wstring errMessage;
        std::wstring sqlState;
        MultiByteToWString(ex.what(), errMessage);
        MultiByteToWString(ex.getSQLStateCStr(), sqlState);

        Logger::WriteLog(L"DBConnector_Error"
            , LOG_LEVEL_ERROR
            , L"%s() - Query: %s, ErrorCode: %d, ErrorMessage: %s, SQLState: %s"
            , __FUNCTIONW__
            , _queryw
            , ex.getErrorCode()
            , errMessage.c_str()
            , sqlState.c_str());

        throw ex;
    }

    return res;
}
void DBConnector::ClearQuery(sql::ResultSet* res)
{
    delete res;
}
void DBConnector::ProfileBegin()
{
    //--------------------------------------------------------------------
    // 프로파일링 시작
    //--------------------------------------------------------------------
    QueryPerformanceCounter(&_profile.lStartTime);
}
void DBConnector::ProfileEnd()
{
    //--------------------------------------------------------------------
    // 프로파일링 종료
    //--------------------------------------------------------------------
    LARGE_INTEGER lEndTime;
    QueryPerformanceCounter(&lEndTime);

    //--------------------------------------------------------------------
    // 프로파일링한 결과를 메모리에 기록
    //--------------------------------------------------------------------
    DWORD64 between = lEndTime.QuadPart - _profile.lStartTime.QuadPart;
    if (_profile.iMin > between || _profile.iMin == 0)
        _profile.iMin = between;

    if (_profile.iMax < between)
        _profile.iMax = between;

    _profile.iTotalTime += between;
    _profile.iCall++;

    //--------------------------------------------------------------------
    // SlowQuery 여부 확인
    //--------------------------------------------------------------------
    LONGLONG millisecond = _freq.QuadPart / 1000;
    double queryTime = (double)between / millisecond;
    if (queryTime >= SLOWQUERY_TIME)
    {
        Logger::WriteLog(L"DBConnector_SlowQuery"
            , LOG_LEVEL_SYSTEM
            , L"SlowQuery - Query: %s, QueryTime: %.4lfms"
            , _queryw
            , queryTime);
    }

    //--------------------------------------------------------------------
    // 프로파일 데이터 파일 기록 주기 확인
    //--------------------------------------------------------------------
    DWORD currentTime = timeGetTime();
    if (currentTime >= _lastProfileTime + PROFILE_TERM)
    {
        ProfileDataOutText();
        _lastProfileTime = currentTime;
    }
}
void DBConnector::ProfileDataOutText()
{
    //--------------------------------------------------------------------
    // 프로파일링한 데이터를 파일에 기록
    //--------------------------------------------------------------------
    LONGLONG millisecond = _freq.QuadPart / 1000;
    double max = (double)_profile.iMax / millisecond;
    double min = (double)_profile.iMin / millisecond;
    unsigned long long call = _profile.iCall;
    double totaltime = _profile.iTotalTime;
    double average = (double)(totaltime / call) / millisecond;

    Logger::WriteLog(L"DBConnector_Profile"
        , LOG_LEVEL_SYSTEM
        , L"Profile - ThreadID: 0x%04X, Average: %.4lfms, Max: %.4lfms, Min: %.4lfms, Call: %I64d"
        , GetCurrentThreadId()
        , average
        , max
        , min
        , call);
}
