#pragma once
#include "../../Lib/Network/include/NetServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/Lock.h"
#include <unordered_set>

typedef DWORD64 SESSION_ID;

class MonitorServer : public Jay::NetServer
{
public:
	MonitorServer();
	~MonitorServer();
public:
	void Update(int serverNo, BYTE dataType, int dataValue, int timeStamp);
private:
	bool OnConnectionRequest(const wchar_t* ipaddress, int port) override;
	void OnClientJoin(DWORD64 sessionID) override;
	void OnClientLeave(DWORD64 sessionID) override;
	void OnRecv(DWORD64 sessionID, Jay::NetPacket* packet) override;
	void OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam) override;
private:
	bool ValidationKey(const char* loginSessionKey);
	void SendBroadcast(Jay::NetPacket* packet);
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_Login(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	std::unordered_set<SESSION_ID> _clientTable;
	Jay::SRWLock _tableLock;
};
