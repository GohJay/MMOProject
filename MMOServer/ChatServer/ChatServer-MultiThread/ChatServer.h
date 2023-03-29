#pragma once
#include "../../Lib/Network/include/NetServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/LFQueue.h"
#include "../../Common/LFObjectPool_TLS.h"
#include "../../Common/Lock.h"
#include "Define.h"
#include "Object.h"
#include <unordered_map>
#include <list>
#include <thread>
#include <atomic>

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
private:
	bool OnConnectionRequest(const wchar_t* ipaddress, int port) override;
	void OnClientJoin(DWORD64 sessionID) override;
	void OnClientLeave(DWORD64 sessionID) override;
	void OnRecv(DWORD64 sessionID, Jay::NetPacket* packet) override;
	void OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam) override;
private:
	bool Initial();
	void Release();
private:
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
	void LockSectorAround(SECTOR_AROUND* sectorAround);
	void UnLockSectorAround(SECTOR_AROUND* sectorAround);
private:
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_ChatLogin(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_ChatSectorMove(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_ChatMessage(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	std::unordered_map<SESSION_ID, PLAYER*> _playerMap;
	Jay::LFObjectPool_TLS<PLAYER> _playerPool;
	Jay::SRWLock _playerLock;
	std::atomic<int> _loginPlayerCount;
	std::list<PLAYER*> _sectorList[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	Jay::SRWLock _sectorLockTable[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
};
