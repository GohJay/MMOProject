#pragma once
#include "GameServer.h"
#include "PlayerObject.h"
#include "../../Common/DBConnector.h"
#include "../../Lib/Network/include/NetContent.h"
#include <thread>
#include <atomic>
#include <unordered_map>
#include <cpp_redis/cpp_redis>

typedef DWORD64 SESSION_ID;

class AuthContent : public Jay::NetContent
{
public:
	AuthContent(GameServer* server);
	~AuthContent();
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
private:
	bool Initial();
	void Release();
	void ManagementThread();
	void UpdateFPS();
	bool ValdateSessionToken(std::string& key, std::string& token);
	bool GetExistPlayerInfo(PlayerObject* player);
	void SetDefaultPlayerInfo(PlayerObject* player, BYTE characterType);
	void MoveGameThread(PlayerObject* player);
private:
	bool PacketProc(DWORD64 sessionID, Jay::NetPacket* packet, WORD type);
	bool PacketProc_GameLogin(DWORD64 sessionID, Jay::NetPacket* packet);
	bool PacketProc_CharacterSelect(DWORD64 sessionID, Jay::NetPacket* packet);
private:
	GameServer* _server;
	std::unordered_map<SESSION_ID, PlayerObject*> _playerMap;
	std::atomic<int> _oldFPS;
	std::atomic<int> _curFPS;
	std::thread _managementThread;
	Jay::DBConnector _gamedb;
	cpp_redis::client _memorydb;
	bool _stopSignal;
};
