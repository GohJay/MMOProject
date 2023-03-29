#pragma once
#include "Object.h"
#include "../../Lib/Network/include/NetServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/Lock.h"
#include <thread>
#include <unordered_map>

typedef DWORD64 SESSION_ID;

class MonitorServer : public Jay::NetServer
{
private:
	struct USER
	{
		bool login;
		DWORD connectionTime;
	};
public:
	MonitorServer();
	~MonitorServer();
	bool Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec = 0, bool nagle = true);
	void Stop();
	void Update(int serverNo, BYTE dataType, int dataValue, int timeStamp);
private:
	bool OnConnectionRequest(const wchar_t* ipaddress, int port) override;
	void OnClientJoin(DWORD64 sessionID) override;
	void OnClientLeave(DWORD64 sessionID) override;
	void OnRecv(DWORD64 sessionID, Jay::NetPacket* packet) override;
	void OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam) override;
private:
	bool Initial();
	void Release();
	void ManagementThread();
private:
	void TimeoutProc();
	USER* NewUser(DWORD64 sessionID);
	void DeleteUser(DWORD64 sessionID);
	bool FindUser(DWORD64 sessionID, USER** user);
	bool ValidationKey(const char* loginSessionKey);
	void SendBroadcast(Jay::NetPacket* packet);
private:
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_Login(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	std::thread _managementThread;
	std::unordered_map<SESSION_ID, USER*> _userMap;
	Jay::SRWLock _mapLock;
	bool _stopSignal;
};
