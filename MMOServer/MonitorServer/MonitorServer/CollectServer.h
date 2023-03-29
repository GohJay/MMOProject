#pragma once
#include "MonitorServer.h"
#include "Object.h"
#include "DBJob.h"
#include "Object.h"
#include "../../Lib/Network/include/LanServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/Lock.h"
#include "../../Common/DBConnector.h"
#include "../../Common/MessageQueue.h"
#include <unordered_map>
#include <thread>

typedef DWORD64 SESSION_ID;
typedef INT SERVER_NO;

class CollectServer : public Jay::LanServer
{
private:
	struct USER
	{
		bool login;
		int serverNo;
		DWORD connectionTime;
	};
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
	void ManagementThread();
private:
	void TimeoutProc();
	USER* NewUser(DWORD64 sessionID);
	void DeleteUser(DWORD64 sessionID);
	bool FindUser(DWORD64 sessionID, USER** user);
	DATA* NewData(int serverNo, BYTE dataType);
	void DeleteData(int serverNo);
	bool FindData(int serverNo, BYTE dataType, DATA** data);
private:
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_Login(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_DataUpdate(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	MonitorServer* _monitor;
	std::thread _dbWriteThread;
	std::thread _managementThread;
	std::unordered_map<SESSION_ID, USER*> _userMap;
	std::unordered_multimap<SERVER_NO, DATA*> _dataMap;
	Jay::SRWLock _mapLock;
	Jay::DBConnector _logdb;
	Jay::MessageQueue<IDBJob*> _dbJobQ;
	HANDLE _hJobEvent;
	HANDLE _hExitEvent;
};
