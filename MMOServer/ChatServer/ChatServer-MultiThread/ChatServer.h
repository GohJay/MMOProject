#pragma once
#include "../../Lib/Network/include/NetServer.h"
#include "../../Common/CommonProtocol.h"
#include "../../Common/LockFreeQueue.h"
#include "../../Common/ObjectPool_TLS.h"
#include "../../Common/Lock.h"
#include "Define.h"
#include "Object.h"
#include <unordered_map>
#include <list>
#include <thread>

typedef DWORD64 SESSION_ID;

class ChatServer : public Jay::NetServer
{
public:
	ChatServer();
	~ChatServer();
public:
	bool Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec = 0, bool nagle = true);
	void Stop();
	int GetCharacterCount();
	int GetUseCharacterPool();
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
	CHARACTER* NewCharacter(DWORD64 sessionID);
	void DeleteCharacter(DWORD64 sessionID);
	CHARACTER* FindCharacter(DWORD64 sessionID);
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_ChatLogin(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_ChatSectorMove(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_ChatMessage(DWORD64 sessionID, Jay::NetPacket* packet);
	bool IsMovableCharacter(int sectorX, int sectorY);
	void AddCharacter_Sector(CHARACTER* character, int sectorX, int sectorY);
	void RemoveCharacter_Sector(CHARACTER* character);
	void UpdateCharacter_Sector(CHARACTER* character, int sectorX, int sectorY);
	void GetSectorAround(int sectorX, int sectorY, SECTOR_AROUND* sectorAround);
	void SendSectorOne(Jay::NetPacket* packet, int sectorX, int sectorY);
	void SendSectorAround(CHARACTER* character, Jay::NetPacket* packet);
	void LockSectorAround(SECTOR_AROUND* sectorAround);
	void UnLockSectorAround(SECTOR_AROUND* sectorAround);
private:
	std::unordered_map<SESSION_ID, CHARACTER*> _characterMap;
	Jay::SRWLock _characterMapLock;
	Jay::ObjectPool_TLS<CHARACTER> _characterPool;
	std::list<CHARACTER*> _sectorList[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	Jay::SRWLock _sectorLockTable[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
};
