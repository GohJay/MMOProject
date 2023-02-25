#include "stdafx.h"
#include "CollectServer.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "../../Common/Logger.h"
#include "../../Common/CrashDump.h"
#include "../../Lib/Network/include/Error.h"
#pragma comment(lib, "../../Lib/Network/lib64/Network.lib")
#pragma comment(lib, "../../Lib/MySQL/lib64/vs14/mysqlcppconn-static.lib")

using namespace Jay;

CollectServer::CollectServer(MonitorServer* monitor) : _monitor(monitor)
{
}
CollectServer::~CollectServer()
{
}
bool CollectServer::Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, int timeoutSec, bool nagle)
{
	//--------------------------------------------------------------------
	// Initial
	//--------------------------------------------------------------------
	if (!Initial())
		return false;

	//--------------------------------------------------------------------
	// Network IO Start
	//--------------------------------------------------------------------
	if (!LanServer::Start(ipaddress, port, workerCreateCnt, workerRunningCnt, sessionMax, timeoutSec, nagle))
	{
		Release();
		return false;
	}

	return true;
}
void CollectServer::Stop()
{
	//--------------------------------------------------------------------
	// Network IO Stop
	//--------------------------------------------------------------------
	LanServer::Stop();

	//--------------------------------------------------------------------
	// Release
	//--------------------------------------------------------------------
	Release();
}
bool CollectServer::OnConnectionRequest(const wchar_t* ipaddress, int port)
{
	return true;
}
void CollectServer::OnClientJoin(DWORD64 sessionID)
{
}
void CollectServer::OnClientLeave(DWORD64 sessionID)
{	
	LockGuard<SRWLock> _lockGuard(&_mapLock);
	auto serverIter = _serverMap.find(sessionID);
	if (serverIter == _serverMap.end())
		return;

	int serverNo = serverIter->second;
	_serverMap.erase(serverIter);

	auto dataRange = _dataMap.equal_range(serverNo);
	for (auto dataIter = dataRange.first; dataIter != dataRange.second;)
	{
		DATA* data = dataIter->second;
		delete data;
		dataIter = _dataMap.erase(dataIter);
	}
}
void CollectServer::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 수신 받은 메시지 처리
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		Disconnect(sessionID);
}
void CollectServer::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------
	// Network IO Error 로깅
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Monitor"
		, LOG_LEVEL_ERROR
		, L"func: %s, line: %d, error: %d, wParam: %llu, lParam: %llu"
		, funcname, linenum, errcode, wParam, lParam);

	//--------------------------------------------------------------------
	// Fatal Error 일 경우 크래시와 함께 메모리 덤프를 남긴다.
	//--------------------------------------------------------------------
	if (errcode >= NET_FATAL_INVALID_SIZE)
		CrashDump::Crash();
}
bool CollectServer::Initial()
{
	_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (_hExitEvent == NULL)
		return false;

	_hJobEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (_hJobEvent == NULL)
	{
		CloseHandle(_hExitEvent);
		return false;
	}

	//--------------------------------------------------------------------
	// Database Connect
	//--------------------------------------------------------------------
	_logdb.Connect(ServerConfig::GetDatabaseIP()
		, ServerConfig::GetDatabasePort()
		, ServerConfig::GetDatabaseUser()
		, ServerConfig::GetDatabasePassword()
		, ServerConfig::GetDatabaseSchema());

	//--------------------------------------------------------------------
	// DBWriteThread Begin
	//--------------------------------------------------------------------
	_dbWriteThread = std::thread(&CollectServer::DBWriteThread, this);
	return true;
}
void CollectServer::Release()
{
	//--------------------------------------------------------------------
	// DBWriteThread End
	//--------------------------------------------------------------------
	SetEvent(_hExitEvent);
	_dbWriteThread.join();

	//--------------------------------------------------------------------
	// Database Disconnect
	//--------------------------------------------------------------------
	_logdb.Disconnect();

	CloseHandle(_hExitEvent);
	CloseHandle(_hJobEvent);
}
void CollectServer::DBWriteThread()
{
	HANDLE hHandle[2] = { _hJobEvent, _hExitEvent };
	DWORD ret;
	while (1)
	{
		ret = WaitForMultipleObjects(2, hHandle, FALSE, INFINITE);
		if ((ret - WAIT_OBJECT_0) != 0)
			break;

		IDBJob* job;
		while (_dbJobQ.size() > 0)
		{
			_dbJobQ.Dequeue(job);
			job->Exec(&_logdb);
			delete job;
		}
	}
}
bool CollectServer::FindServerNo(DWORD64 sessionID, int* serverNo)
{
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	auto iter = _serverMap.find(sessionID);
	if (iter != _serverMap.end())
	{
		*serverNo = iter->second;
		return true;
	}
	return false;
}
bool CollectServer::FindData(int serverNo, BYTE dataType, DATA** data)
{
	DATA_FINDER finder(dataType);
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	auto range = _dataMap.equal_range(serverNo);
	auto iter = std::find_if(range.first, range.second, finder);
	if (iter != range.second)
	{
		*data = iter->second;
		return true;
	}
	return false;
}
void CollectServer::UpdateData(int serverNo, BYTE dataType, int dataValue, DATA* data)
{
	//--------------------------------------------------------------------
	// 모니터링 데이터 갱신
	//--------------------------------------------------------------------
	if (data->iMin > dataValue || data->iMin == 0)
		data->iMin = dataValue;
	if (data->iMax < dataValue)
		data->iMax = dataValue;
	data->totalData += dataValue;
	data->iCall++;

	//--------------------------------------------------------------------
	// 주기에 따라 DB 에 모니터링 데이터 저장
	//--------------------------------------------------------------------
	DWORD currentTime = timeGetTime();
	if (currentTime - data->lastLogTime >= dfDATABASE_LOG_WRITE_TERM)
	{
		DBMonitoringLog* dbLog = new DBMonitoringLog(serverNo, dataType, data->totalData / data->iCall, data->iMin, data->iMax);
		_dbJobQ.Enqueue(dbLog);
		SetEvent(_hJobEvent);

		data->totalData = 0;
		data->iMin = 0;
		data->iMax = 0;
		data->iCall = 0;
		data->lastLogTime = currentTime;
	}
}
bool CollectServer::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// 수신 메시지 타입에 따른 분기 처리
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		return PacketProc_Login(sessionID, packet);
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		return PacketProc_DataUpdate(sessionID, packet);
	default:
		break;
	}
	return false;
}
bool CollectServer::PacketProc_Login(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 로그인 메시지 처리
	//--------------------------------------------------------------------
	int serverNo;
	(*packet) >> serverNo;

	LockGuard<SRWLock> lockGuard(&_mapLock);
	auto iter = _dataMap.find(sessionID);
	if (iter == _dataMap.end())
	{
		_serverMap.insert({ sessionID, serverNo });
		return true;
	}
	return false;
}
bool CollectServer::PacketProc_DataUpdate(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 모니터링 데이터 업데이트 메시지 처리
	//--------------------------------------------------------------------
	BYTE dataType;
	int dataValue;
	int timeStamp;
	(*packet) >> dataType;
	(*packet) >> dataValue;
	(*packet) >> timeStamp;

	//--------------------------------------------------------------------
	// 모니터링 데이터 찾기
	//--------------------------------------------------------------------
	int serverNo;
	if (!FindServerNo(sessionID, &serverNo))
		return false;

	DATA* data;
	if (!FindData(serverNo, dataType, &data))
	{
		data = new DATA(dataType);
		data->lastLogTime = timeGetTime();

		_mapLock.Lock();
		_dataMap.insert({ serverNo, data });
		_mapLock.UnLock();
	}

	//--------------------------------------------------------------------
	// 모니터링 데이터 갱신
	//--------------------------------------------------------------------
	UpdateData(serverNo, dataType, dataValue, data);

	//--------------------------------------------------------------------
	// 모니터링 클라이언트에게 데이터 갱신 알림
	//--------------------------------------------------------------------
	_monitor->Update(serverNo, dataType, dataValue, timeStamp);
	return true;
}
