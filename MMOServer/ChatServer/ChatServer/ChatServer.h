#pragma once
#include "../../Lib/Network/include/NetServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/ObjectPool.h"
#include "../../Common/LFObjectPool_TLS.h"
#include "../../Common/LFQueue.h"
#include "../../Common/MessageQueue.h"
#include "Define.h"
#include "Object.h"
#include "Job.h"
#include <thread>
#include <unordered_map>
#include <cpp_redis/cpp_redis>

typedef DWORD64 SESSION_ID;

class ChatServer : public Jay::NetServer
{
public:
	ChatServer();
	~ChatServer();
public:
	bool Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec = 0, bool nagle = true);
	void Stop();
	int GetPlayerCount();
	int GetLoginPlayerCount();
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
	void AuthThread();
	void ManagementThread();
private:
	void UpdateTPS();
	void JoinProc(DWORD64 sessionID);
	void RecvProc(DWORD64 sessionID, Jay::NetPacket* packet);
	void LeaveProc(DWORD64 sessionID);
	void LoginProc(DWORD64 sessionID, bool result);
	void AuthProc(DWORD64 sessionID, __int64 accountNo, char* token);
	void NewPlayer(DWORD64 sessionID);
	void DeletePlayer(DWORD64 sessionID);
	PLAYER* FindPlayer(DWORD64 sessionID);
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
	std::unordered_map<SESSION_ID, PLAYER*> _playerMap;
	Jay::ObjectPool<PLAYER> _playerPool;
	Jay::LFQueue<CHAT_JOB*> _chatJobQ;
	Jay::LFQueue<AUTH_JOB*> _authJobQ;
	Jay::LFObjectPool_TLS<CHAT_JOB> _chatJobPool;
	Jay::LFObjectPool_TLS<AUTH_JOB> _authJobPool;
	int _loginPlayerCount;
	HANDLE _hChatJobEvent;
	HANDLE _hAuthJobEvent;
	HANDLE _hExitEvent;
	std::thread _updateThread;
	std::thread _authThread;
	std::thread _managementThread;
	std::atomic<int> _oldUpdateTPS;
	std::atomic<int> _curUpdateTPS;
	std::list<PLAYER*> _sectorList[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	cpp_redis::client _memorydb;
};
