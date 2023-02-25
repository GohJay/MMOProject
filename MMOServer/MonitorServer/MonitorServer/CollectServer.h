#pragma once
#include "MonitorServer.h"
#include "MonitoringData.h"
#include "DBJob.h"
#include "../../Lib/Network/include/LanServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/Lock.h"
#include "../../Common/DBConnector.h"
#include "../../Common/Lock.h"
#include <unordered_map>
#include <thread>

typedef DWORD64 SESSION_ID;
typedef int SERVER_NO;

class CollectServer : public Jay::LanServer
{
public:
	CollectServer(MonitorServer* monitor);
	~CollectServer();
public:
	bool Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, int timeoutSec = 0, bool nagle = true);
	void Stop();
private:
	bool OnConnectionRequest(const wchar_t* ipaddress, int port) override;
	void OnClientJoin(DWORD64 sessionID) override;
	void OnClientLeave(DWORD64 sessionID) override;
	void OnRecv(DWORD64 sessionID, Jay::NetPacket* packet) override;
	void OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam) override;
private:
	bool Initial();
	void Release();
	void DBWriteThread();
private:
	bool FindServerNo(DWORD64 sessionID, int* serverNo);
	bool FindData(int serverNo, BYTE dataType, DATA** data);
	void UpdateData(int serverNo, BYTE dataType, int dataValue, DATA* data);
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_Login(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_DataUpdate(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	MonitorServer* _monitor;
	std::thread _dbWriteThread;
	std::unordered_map<SESSION_ID, SERVER_NO> _serverMap;
	std::unordered_multimap<SERVER_NO, DATA*> _dataMap;
	Jay::SRWLock _mapLock;
	Jay::DBConnector _logdb;
	Jay::LockFreeQueue<IDBJob*> _dbJobQ;
	HANDLE _hJobEvent;
	HANDLE _hExitEvent;
};
