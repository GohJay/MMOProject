#include "stdafx.h"
#include "AuthContent.h"
#include "ServerConfig.h"
#include "Packet.h"
#include "ObjectManager.h"
#include "GameData.h"
#include <string>
#include "../../Common/CommonProtocol.h"
#include "../../Common/StringUtil.h"
#include "../../Common/CrashDump.h"
#pragma comment(lib, "../../Lib/MySQL/lib64/vs14/mysqlcppconn-static.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/tacopie.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/cpp_redis.lib")

using namespace Jay;

AuthContent::AuthContent(GameServer* server) : _server(server), _stopSignal(false)
{
	//--------------------------------------------------------------------
	// NetworkLib �� ���� ������ ����
	//--------------------------------------------------------------------	
	_server->AttachContent(this, CONTENT_ID_AUTH, FRAME_INTERVAL_AUTH, true);
}
AuthContent::~AuthContent()
{
}
int AuthContent::GetPlayerCount()
{
	return _playerMap.size();
}
int AuthContent::GetFPS()
{
	return _oldFPS;
}
void AuthContent::OnStart()
{
	//--------------------------------------------------------------------
	// Initial
	//--------------------------------------------------------------------
	Initial();
}
void AuthContent::OnStop()
{
	//--------------------------------------------------------------------
	// Release
	//--------------------------------------------------------------------
	Release();
}
void AuthContent::OnUpdate()
{
	_curFPS++;
}
void AuthContent::OnClientJoin(DWORD64 sessionID)
{
}
void AuthContent::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// ���� ���� �޽��� ó��
	//--------------------------------------------------------------------
	WORD type;
	(*packet) >> type;

	if (!PacketProc(sessionID, packet, type))
		_server->Disconnect(sessionID);
}
void AuthContent::OnClientLeave(DWORD64 sessionID)
{
	auto iter = _playerMap.find(sessionID);
	PlayerObject* player = iter->second;
	_playerMap.erase(iter);
	PlayerObject::Free(player);
}
void AuthContent::OnContentEnter(DWORD64 sessionID, WPARAM wParam, LPARAM lParam)
{
	PlayerObject* player = PlayerObject::Alloc();
	_playerMap.insert({ sessionID, player });
}
void AuthContent::OnContentExit(DWORD64 sessionID)
{
	_playerMap.erase(sessionID);
}
bool AuthContent::Initial()
{
	//--------------------------------------------------------------------
	// Redis Connect
	//--------------------------------------------------------------------	
	std::string redisIP;
	UnicodeToString(ServerConfig::GetRedisIP(), redisIP);
	_memorydb.connect(redisIP, ServerConfig::GetRedisPort());

	//--------------------------------------------------------------------
	// Database Connect
	//--------------------------------------------------------------------	
	_gamedb.Connect(ServerConfig::GetDatabaseIP()
		, ServerConfig::GetDatabasePort()
		, ServerConfig::GetDatabaseUser()
		, ServerConfig::GetDatabasePassword()
		, ServerConfig::GetDatabaseSchema());

	//--------------------------------------------------------------------
	// Thread Begin
	//--------------------------------------------------------------------
	_managementThread = std::thread(&AuthContent::ManagementThread, this);
	return true;
}
void AuthContent::Release()
{
	//--------------------------------------------------------------------
	// Thread End
	//--------------------------------------------------------------------
	_stopSignal = true;
	_managementThread.join();

	//--------------------------------------------------------------------
	// Database Disconnect
	//--------------------------------------------------------------------
	_gamedb.Disconnect();

	//--------------------------------------------------------------------
	// Redis Disconnect
	//--------------------------------------------------------------------
	_memorydb.disconnect();
}
void AuthContent::ManagementThread()
{
	while (!_stopSignal)
	{
		Sleep(1000);
		UpdateFPS();
	}
}
void AuthContent::UpdateFPS()
{
	_oldFPS.exchange(_curFPS.exchange(0));
}
bool AuthContent::ValdateSessionToken(std::string& key, std::string& token)
{
	std::future<cpp_redis::reply> future = _memorydb.get(key);
	_memorydb.sync_commit();

	//--------------------------------------------------------------------
	// ���� ��ū ����
	//--------------------------------------------------------------------
	cpp_redis::reply reply = future.get();
	if (reply.is_string() && reply.as_string().compare(token) == 0)
	{
		//--------------------------------------------------------------------
		// Redis ���� ���� �Ϸ��� ��ū ����
		//--------------------------------------------------------------------
		std::vector<std::string> keys(1);
		keys.emplace_back(std::move(key));
		_memorydb.del(keys);
		_memorydb.sync_commit();

		return true;
	}

	return false;
}
bool AuthContent::GetExistPlayerInfo(PlayerObject* player)
{
	bool ret = false;

	std::wstring nickname;
	BYTE characterType;
	float posX;
	float posY;
	USHORT rotation;
	int cristal;
	int hp;
	INT64 exp;
	USHORT level;
	bool die;

	//--------------------------------------------------------------------
	// DB ���� ���� �÷��̾��� �ɷ�ġ ��ȸ
	//--------------------------------------------------------------------
	sql::ResultSet* res1;
	sql::ResultSet* res2;
	res1 = _gamedb.ExecuteQuery(L"SELECT charactertype, posx, posy, rotation, cristal, hp, exp, level, die FROM gamedb.character WHERE accountno = %lld;", player->GetAccountNo());
	if (res1->next())
	{
		characterType = res1->getInt(1);
		posX = res1->getDouble(2);
		posY = res1->getDouble(3);
		rotation = res1->getInt(4);
		cristal = res1->getInt(5);
		hp = res1->getInt(6);
		exp = res1->getInt64(7);
		level = res1->getInt(8);
		die = res1->getBoolean(9);

		//--------------------------------------------------------------------
		// DB ���� �÷��̾� �г��� ��ȸ
		//--------------------------------------------------------------------
		res2 = _gamedb.ExecuteQuery(L"SELECT usernick FROM accountdb.account WHERE accountno = %lld;", player->GetAccountNo());
		if (res2->next())
			MultiByteToWString(res2->getString(1).c_str(), nickname);
		_gamedb.ClearQuery(res2);

		player->Init(&nickname[0], characterType, posX, posY, rotation, cristal, hp, exp, level, die);
		ret = true;
	}
	_gamedb.ClearQuery(res1);

	return ret;
}
void AuthContent::SetDefaultPlayerInfo(PlayerObject* player, BYTE characterType)
{
	std::wstring nickname;
	int tileX;
	int tileY;
	USHORT rotation;
	INT64 cristal;
	int hp;
	int exp;
	int level;

	//--------------------------------------------------------------------
	// DB ���� �÷��̾� �г��� ��ȸ
	//--------------------------------------------------------------------
	sql::ResultSet* res = _gamedb.ExecuteQuery(L"SELECT usernick FROM accountdb.account WHERE accountno = %lld;", player->GetAccountNo());
	if (res->next())
		MultiByteToWString(res->getString(1).c_str(), nickname);

	//--------------------------------------------------------------------
	// �ű� �÷��̾ �⺻ �ɷ�ġ�� ����
	//--------------------------------------------------------------------
	tileX = (dfPLAYER_TILE_X_RESPAWN_CENTER - 10) + (rand() % 21);
	tileY = (dfPLAYER_TILE_Y_RESPAWN_CENTER - 15) + (rand() % 31);
	rotation = rand() % 360;
	cristal = 0;
	hp = dfPLAYER_HP_DEFAULT;
	exp = 0;
	level = 1;

	//--------------------------------------------------------------------
	// DB �� �ű� �÷��̾� �ɷ�ġ ����
	//--------------------------------------------------------------------
	_gamedb.ExecuteUpdate(L"INSERT INTO gamedb.character(accountno, charactertype, posx, posy, tilex, tiley, rotation, cristal, hp, exp, level, die) VALUES(%lld, %d, %f, %f, %d, %d, %d, %d, %d, %lld, %d, %d);"
		, player->GetAccountNo()
		, characterType
		, (float)tileX / 2
		, (float)tileY / 2
		, tileX
		, tileY
		, rotation
		, cristal
		, hp
		, exp
		, level
		, FALSE);

	player->Init(&nickname[0], characterType, tileX / 2, tileY / 2, rotation, cristal, hp, exp, level, false);
}
void AuthContent::MoveGameThread(PlayerObject* player)
{
	//--------------------------------------------------------------------
	// �ش� �÷��̾ ���� ������� �̵�
	//--------------------------------------------------------------------
	_server->MoveContent(player->GetSessionID(), CONTENT_ID_GAME, (WPARAM)player, NULL);
}
bool AuthContent::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// ���� �޽��� Ÿ�Կ� ���� �б� ó��
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_CS_GAME_REQ_LOGIN:
		return PacketProc_GameLogin(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT:
		return PacketProc_CharacterSelect(sessionID, packet);
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		return true;
	default:		
		break;
	}
	return false;
}
bool AuthContent::PacketProc_GameLogin(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// �α��� �޽��� ó��
	//--------------------------------------------------------------------
	INT64 accountNo;
	char sessionKey[64];
	int version;
	(*packet) >> accountNo;
	if (packet->GetData(sessionKey, sizeof(sessionKey)) != sizeof(sessionKey))
		return false;
	(*packet) >> version;

	//--------------------------------------------------------------------
	// �ߺ� �α��� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (player->IsLogin())
		return false;

	//--------------------------------------------------------------------
	// Redis ���� ���� ����
	//--------------------------------------------------------------------
	std::string key = std::to_string(accountNo) + "_game";
	std::string token = std::string(sessionKey, sizeof(sessionKey));

	BYTE status;
	if (ValdateSessionToken(key, token))
	{
		player->Auth(sessionID, accountNo);

		if (GetExistPlayerInfo(player))
		{
			MoveGameThread(player);
			status = dfGAME_LOGIN_OK;
		}
		else
			status = dfGAME_LOGIN_NOPLAYER;
	}
	else
		status = dfGAME_LOGIN_FAIL;

	//--------------------------------------------------------------------
	// �ش� �������� �α��� �Ϸ� �޽��� ������
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	Packet::MakeGameLogin(resPacket, status, accountNo);
	_server->SendPacket(sessionID, resPacket);

	NetPacket::Free(resPacket);
	return true;
}
bool AuthContent::PacketProc_CharacterSelect(DWORD64 sessionID, Jay::NetPacket* packet)
{
	BYTE characterType;
	(*packet) >> characterType;

	//--------------------------------------------------------------------
	// �α��� ���� Ȯ��
	//--------------------------------------------------------------------
	PlayerObject* player = _playerMap[sessionID];
	if (!player->IsLogin())
		return false;

	BYTE status;
	switch (characterType)
	{
	case GOLEM:
	case KNIGHT:
	case ORC:
	case ELF:
	case ARCHER:
		SetDefaultPlayerInfo(player, characterType);
		MoveGameThread(player);
		status = TRUE;
		break;
	default:
		status = FALSE;
		break;
	}

	//--------------------------------------------------------------------
	// �ش� �������� ĳ���� ���� �Ϸ� �޽��� ������
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	Packet::MakeCharacterSelect(resPacket, status);
	_server->SendPacket(sessionID, resPacket);

	NetPacket::Free(resPacket);

	if (status == FALSE)
		return false;

	return true;
}
