#pragma once
#include "Define.h"
#include "GameServer.h"
#include "PlayerObject.h"
#include "MonsterObject.h"
#include "CristalObject.h"
#include "GameData.h"
#include "Sector.h"
#include "IDBJob.h"
#include "../../Lib/Network/include/NetContent.h"
#include "../../Common/MessageQueue.h"
#include "../../Common/DBConnector.h"
#include <thread>
#include <atomic>
#include <unordered_map>
#include <list>

typedef DWORD64 SESSION_ID;

class GameContent : public Jay::NetContent
{
public:
	GameContent(GameServer* server);
	~GameContent();
public:
	int GetPlayerCount();
	int GetFPS();
private:
	void OnStart() override;
	void OnStop() override;
	void OnUpdate() override;
	void OnRecv(DWORD64 sessionID, Jay::NetPacket* packet) override;
	void OnClientJoin(DWORD64 sessionID) override;
	void OnClientLeave(DWORD64 sessionID) override;
	void OnContentEnter(DWORD64 sessionID, WPARAM wParam, LPARAM lParam) override;
	void OnContentExit(DWORD64 sessionID) override;
public:
	void SendUnicast(PlayerObject* player, Jay::NetPacket* packet);
	void SendSectorOne(BaseObject* exclusion, Jay::NetPacket* packet, int sectorX, int sectorY);
	void SendSectorAround(BaseObject* object, Jay::NetPacket* packet, bool sendMe = false);
	void GetSectorAround(int sectorX, int sectorY, SECTOR_AROUND* sectorAround);
	void GetUpdateSectorAround(BaseObject* object, SECTOR_AROUND* removeSector, SECTOR_AROUND* addSector);
	void UpdateSectorAround_Player(PlayerObject* player);
	void UpdateSectorAround_Monster(MonsterObject* monster);
	void AddSector(BaseObject* object);
	void RemoveSector(BaseObject* object);
	void AddTile(BaseObject* object);
	void RemoveTile(BaseObject* object);
	bool IsTileOut(int tileX, int tileY);
	std::list<BaseObject*>* GetTile(int tileX, int tileY);
	std::list<BaseObject*>* GetSector(int sectorX, int sectorY);
	DATA_CRISTAL* GetDataCristal(INT64 type);
	DATA_MONSTER* GetDataMonster(INT64 type);
	DATA_PLAYER* GetDataPlayer();
	void CreateCristal(float posX, float posY);
	void DeleteCristal(CristalObject* cristal);
	void DBPostPlayerLogin(PlayerObject* player);
	void DBPostPlayerLogout(PlayerObject* player);
	void DBPostPlayerDie(PlayerObject* player, int minusCristal);
	void DBPostPlayerRestart(PlayerObject* player);
	void DBPostGetCristal(PlayerObject* player, int getCristal);
	void DBPostRecoverHP(PlayerObject* player, int oldHP, int sitTimeSec);
private:
	bool Initial();
	void Release();
	void DBWriteThread();
	void ManagementThread();
	void UpdateFPS();
	void FetchGamedbData();
	void AssignMonsterZone();
private:
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_MoveStart(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_MoveStop(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_Attak1(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_Attak2(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_Pick(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_Sit(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_Restart(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_GameEcho(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	GameServer* _server;
	std::unordered_map<SESSION_ID, PlayerObject*> _playerMap;
	std::unordered_map<INT64, DATA_CRISTAL*> _dataCristalMap;
	std::unordered_map<INT64, DATA_MONSTER*> _dataMonsterMap;
	DATA_PLAYER _dataPlayer;
	std::list<BaseObject*> _tile[dfMAP_TILE_MAX_Y][dfMAP_TILE_MAX_X];
	std::list<BaseObject*> _sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	std::atomic<int> _oldFPS;
	std::atomic<int> _curFPS;
	std::thread _managementThread;
	std::thread _dbWriteThread;
	Jay::DBConnector _gamedb;
	Jay::MessageQueue<IDBJob*> _dbJobQ;
	HANDLE _hDBJobEvent;
	HANDLE _hExitEvent;
};
