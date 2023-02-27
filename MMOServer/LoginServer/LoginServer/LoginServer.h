#pragma once
#include "../../Lib/Network/include/NetServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/LockFreeQueue.h"
#include "../../Common/Lock.h"
#include "../../Common/DBConnector_TLS.h"
#include <unordered_set>
#include <cpp_redis/cpp_redis>
#include <thread>
#include <atomic>

typedef std::wstring WHITE_IP;

class LoginServer : public Jay::NetServer
{
public:
	LoginServer();
	~LoginServer();
public:
	bool Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec = 0, bool nagle = true);
	void Stop();
	void SwitchServiceMode();
	int GetAuthTPS();
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
	void UpdateTPS();
private:
	void FetchWhiteIPList();
	bool CheckWhiteIP(const wchar_t* ipaddress);
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_Login(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	Jay::DBConnector_TLS _accountdb;
	std::unordered_set<WHITE_IP> _whiteIPTable;
	std::atomic<int> _oldAuthTPS;
	std::atomic<int> _curAuthTPS;
	std::thread _managementThread;
	cpp_redis::client _memorydb;
	bool _serviceMode;
	bool _stopSignal;
};
