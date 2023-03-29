#pragma once
#include "MainServer.h"
#include "../../Lib/Network/include/NetContent.h"
#include <thread>
#include <atomic>
#include <unordered_set>

typedef DWORD64 SESSION_ID;

class AuthServer : public Jay::NetContent
{
public:
	AuthServer(MainServer* subject);
	~AuthServer();
public:
	int GetPlayerCount();
	int GetFPS();
private:
	void OnUpdate() override;
	void OnClientJoin(DWORD64 sessionID) override;
	void OnClientLeave(DWORD64 sessionID) override;
	void OnContentEnter(DWORD64 sessionID, WPARAM wParam, LPARAM lParam) override;
	void OnContentExit(DWORD64 sessionID) override;
	void OnRecv(DWORD64 sessionID, Jay::NetPacket* packet) override;
private:
	void ManagementThread();
	void UpdateFPS();
private:
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_GameLogin(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	MainServer* _subject;
	std::unordered_set<SESSION_ID> _playerTable;
	std::atomic<int> _oldFPS;
	std::atomic<int> _curFPS;
	std::thread _managementThread;
	bool _stopSignal;
};
