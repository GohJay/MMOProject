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
	NewUser(sessionID);
}
void CollectServer::OnClientLeave(DWORD64 sessionID)
{
	USER* user;
	FindUser(sessionID, &user);

	if (user->login)
		DeleteData(user->serverNo);
	DeleteUser(sessionID);
}
void CollectServer::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ���� ���� �޽��� ó��
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		Disconnect(sessionID);
}
void CollectServer::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------
	// Network IO Error �α�
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Collect"
		, LOG_LEVEL_ERROR
		, L"func: %s, line: %d, error: %d, wParam: %llu, lParam: %llu"
		, funcname, linenum, errcode, wParam, lParam);

	//--------------------------------------------------------------------
	// Fatal Error �� ��� ũ���ÿ� �Բ� �޸� ������ �����.
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
	_managementThread = std::thread(&CollectServer::ManagementThread, this);
	return true;
}
void CollectServer::Release()
{
	//--------------------------------------------------------------------
	// DBWriteThread End
	//--------------------------------------------------------------------
	SetEvent(_hExitEvent);
	_dbWriteThread.join();
	_managementThread.join();

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
		_dbJobQ.Flip();
		while (_dbJobQ.size() > 0)
		{
			_dbJobQ.Dequeue(job);
			job->Exec(&_logdb);
			delete job;
		}
	}
}
void CollectServer::ManagementThread()
{
	DWORD ret;
	while (1)
	{
		ret = WaitForSingleObject(_hExitEvent, 5000);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			return;
		case WAIT_TIMEOUT:
			TimeoutProc();
			break;
		default:
			break;
		}
	}
}
void CollectServer::TimeoutProc()
{
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	DWORD currentTime = timeGetTime();
	for (auto iter = _userMap.begin(); iter != _userMap.end(); ++iter)
	{
		SESSION_ID sessionID = iter->first;
		USER* user = iter->second;
		if (user->login)
			continue;

		if (currentTime - user->connectionTime >= dfNETWORK_NON_LOGIN_USER_TIMEOUT)
			LanServer::Disconnect(sessionID);
	}
}
CollectServer::USER* CollectServer::NewUser(DWORD64 sessionID)
{
	USER* user = new USER;
	user->login = false;
	user->connectionTime = timeGetTime();

	_mapLock.Lock();
	_userMap.insert({ sessionID, user });
	_mapLock.UnLock();

	return user;
}
void CollectServer::DeleteUser(DWORD64 sessionID)
{
	USER* user;

	_mapLock.Lock();
	auto iter = _userMap.find(sessionID);
	user = iter->second;
	_userMap.erase(iter);
	_mapLock.UnLock();

	delete user;
}
bool CollectServer::FindUser(DWORD64 sessionID, USER** user)
{
	LockGuard_Shared<SRWLock> lockGuard(&_mapLock);
	auto iter = _userMap.find(sessionID);
	if (iter != _userMap.end())
	{
		*user = iter->second;
		return true;
	}
	return false;
}
DATA* CollectServer::NewData(int serverNo, BYTE dataType)
{
	DATA* data = new DATA(dataType);
	data->totalData = 0;
	data->iMin = 0;
	data->iMax = 0;
	data->iCall = 0;
	data->lastLogTime = timeGetTime();

	_mapLock.Lock();
	_dataMap.insert({ serverNo, data });
	_mapLock.UnLock();
	return data;
}
void CollectServer::DeleteData(int serverNo)
{
	DATA* data;
	LockGuard<SRWLock> lockGuard(&_mapLock);
	auto range = _dataMap.equal_range(serverNo);
	for (auto iter = range.first; iter != range.second;)
	{
		data = iter->second;
		delete data;
		iter = _dataMap.erase(iter);
	}
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
		return data;
	}
	return false;
}
bool CollectServer::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// ���� �޽��� Ÿ�Կ� ���� �б� ó��
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
	// �α��� �޽��� ó��
	//--------------------------------------------------------------------
	int serverNo;
	(*packet) >> serverNo;

	//--------------------------------------------------------------------
	// �ߺ� �α��� Ȯ��
	//--------------------------------------------------------------------
	USER* user;
	FindUser(sessionID, &user);
	if (user->login)
		return false;

	user->serverNo = serverNo;
	user->login = true;
	return true;
}
bool CollectServer::PacketProc_DataUpdate(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ����͸� ������ ������Ʈ �޽��� ó��
	//--------------------------------------------------------------------
	BYTE dataType;
	int dataValue;
	int timeStamp;
	(*packet) >> dataType;
	(*packet) >> dataValue;
	(*packet) >> timeStamp;

	//--------------------------------------------------------------------
	// �α��� ���� Ȯ��
	//--------------------------------------------------------------------
	USER* user;
	FindUser(sessionID, &user);
	if (!user->login)
		return false;

	//--------------------------------------------------------------------
	// ����͸� ������ ã��
	//--------------------------------------------------------------------
	DATA* data;
	if (!FindData(user->serverNo, dataType, &data))
		data = NewData(user->serverNo, dataType);

	//--------------------------------------------------------------------
	// ����͸� ������ ����
	//--------------------------------------------------------------------
	if (data->iMin > dataValue || data->iMin == 0)
		data->iMin = dataValue;
	if (data->iMax < dataValue)
		data->iMax = dataValue;
	data->totalData += dataValue;
	data->iCall++;

	//--------------------------------------------------------------------
	// �ֱ⿡ ���� DB �� ����͸� ������ ����
	//--------------------------------------------------------------------
	DBMonitoringLog* dbLog;
	DWORD currentTime = timeGetTime();
	if (currentTime - data->lastLogTime >= dfDATABASE_LOG_WRITE_TERM)
	{
		dbLog = new DBMonitoringLog(user->serverNo
			, dataType
			, data->totalData / data->iCall
			, data->iMin
			, data->iMax);

		_dbJobQ.Enqueue(dbLog);
		SetEvent(_hJobEvent);

		data->totalData = 0;
		data->iMin = 0;
		data->iMax = 0;
		data->iCall = 0;
		data->lastLogTime = currentTime;
	}

	//--------------------------------------------------------------------
	// ����͸� Ŭ���̾�Ʈ���� ������ ���� �˸�
	//--------------------------------------------------------------------
	_monitor->Update(user->serverNo, dataType, dataValue, timeStamp);
	return true;
}
