#pragma once
#include "../../Lib/Network/include/NetServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/LockFreeQueue.h"
#include "../../Common/ObjectPool_TLS.h"
#include "Define.h"
#include "Object.h"
#include "ChatJob.h"
#include <unordered_map>
#include <list>
#include <thread>
#include <cpp_redis/cpp_redis>

typedef DWORD64 SESSION_ID;
typedef INT64 ACCOUNT_NO;

class ChatServer : public Jay::NetServer
{
public:
	ChatServer();
	~ChatServer();
public:
	bool Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec = 0, bool nagle = true);
	void Stop();
	int GetUserCount();
	int GetPlayerCount();
	int GetUseUserPool();
	int GetUsePlayerPool();
	int GetJobQueueCount();
	int GetUseJobPool();
	int GetUpdateTPS();
private:
	bool OnConnectionRequest(const wchar_t* ipaddress, int port) override;
	void OnClientJoin(DWORD64 sessionID) override;
	void OnClientLeave(DWORD64 sessionID) override;
	void OnRecv(DWORD64 sessionID, Jay::NetPacket* packet) override;
	void OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam) override;
private:
	bool Initial();
	void Release();
	void UpdateThread();
	void ManagementThread();
	void UpdateTPS();
	void JoinProc(DWORD64 sessionID);
	void RecvProc(DWORD64 sessionID, Jay::NetPacket* packet);
	void LeaveProc(DWORD64 sessionID);
	void LoginProc(DWORD64 sessionID, bool result);
private:
	USER* NewUser(DWORD64 sessionID);
	void DeleteUser(DWORD64 sessionID);
	USER* FindUser(INT64 sessionID);
	PLAYER* NewPlayer(DWORD64 sessionID, INT64 accountNo);
	void DeletePlayer(INT64 accountNo);
	PLAYER* FindPlayer(INT64 accountNo);
	bool IsMovablePlayer(int sectorX, int sectorY);
	void AddPlayer_Sector(PLAYER* player, int sectorX, int sectorY);
	void RemovePlayer_Sector(PLAYER* player);
	void UpdatePlayer_Sector(PLAYER* player, int sectorX, int sectorY);
	void GetSectorAround(int sectorX, int sectorY, SECTOR_AROUND* sectorAround);
	void SendSectorOne(Jay::NetPacket* packet, int sectorX, int sectorY);
	void SendSectorAround(PLAYER* player, Jay::NetPacket* packet);
private:
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_ChatLogin(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_ChatSectorMove(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_ChatMessage(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	std::unordered_map<SESSION_ID, USER*> _userMap;
	std::unordered_map<ACCOUNT_NO, PLAYER*> _playerMap;
	Jay::ObjectPool_TLS<USER> _userPool;
	Jay::ObjectPool_TLS<PLAYER> _playerPool;
	Jay::LockFreeQueue<CHAT_JOB*> _jobQ;
	Jay::ObjectPool_TLS<CHAT_JOB> _jobPool;
	HANDLE _hJobEvent;
	HANDLE _hExitEvent;
	std::atomic<int> _oldUpdateTPS;
	std::atomic<int> _curUpdateTPS;
	std::thread _updateThread;
	std::thread _managementThread;
	std::list<PLAYER*> _sectorList[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	cpp_redis::client _memorydb;
};
