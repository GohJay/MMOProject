#include "stdafx.h"
#include "ChatServer.h"
#include "ServerConfig.h"
#include "Packet.h"
#include <vector>
#include "../../Common/Logger.h"
#include "../../Common/CrashDump.h"
#include "../../Common/StringUtil.h"
#include "../../Lib/Network/include/Error.h"
#include "../../Lib/Network/include/NetException.h"
#pragma comment(lib, "../../Lib/Network/lib64/Network.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/tacopie.lib")
#pragma comment(lib, "../../Lib/Redis/lib64/cpp_redis.lib")

using namespace Jay;

ChatServer::ChatServer() : _userPool(0), _playerPool(0), _jobPool(0)
{
}
ChatServer::~ChatServer()
{
}
bool ChatServer::Start(const wchar_t* ipaddress, int port, int workerCreateCnt, int workerRunningCnt, WORD sessionMax, BYTE packetCode, BYTE packetKey, int timeoutSec, bool nagle)
{
	//--------------------------------------------------------------------
	// Initial
	//--------------------------------------------------------------------
	if (!Initial())
		return false;

	//--------------------------------------------------------------------
	// Network IO Start
	//--------------------------------------------------------------------
	if (!NetServer::Start(ipaddress, port, workerCreateCnt, workerRunningCnt, sessionMax, packetCode, packetKey, timeoutSec, nagle))
	{
		Release();
		return false;
	}

	return true;
}
void ChatServer::Stop()
{
	//--------------------------------------------------------------------
	// Network IO Stop
	//--------------------------------------------------------------------
	NetServer::Stop();

	//--------------------------------------------------------------------
	// Release
	//--------------------------------------------------------------------
	Release();
}
int ChatServer::GetUserCount()
{
	return _userMap.size();
}
int ChatServer::GetPlayerCount()
{
	return _playerMap.size();
}
int ChatServer::GetUseUserPool()
{
	return _userPool.GetUseCount();
}
int ChatServer::GetUsePlayerPool()
{
	return _playerPool.GetUseCount();
}
int ChatServer::GetJobQueueCount()
{
	return _jobQ.size();
}
int ChatServer::GetUseJobPool()
{
	return _jobPool.GetUseCount();
}
int ChatServer::GetUpdateTPS()
{
	return _oldUpdateTPS;
}
bool ChatServer::OnConnectionRequest(const wchar_t* ipaddress, int port)
{
	return true;
}
void ChatServer::OnClientJoin(DWORD64 sessionID)
{
	//--------------------------------------------------------------------
	// 오브젝트풀에서 Job 할당
	//--------------------------------------------------------------------
	CHAT_JOB* job = _jobPool.Alloc();
	job->type = JOB_TYPE_CLIENT_JOIN;
	job->sessionID = sessionID;

	//--------------------------------------------------------------------
	// Job 큐잉
	//--------------------------------------------------------------------
	_jobQ.Enqueue(job);

	//--------------------------------------------------------------------
	// UpdateThread 에게 Job 이벤트 알림
	//--------------------------------------------------------------------
	SetEvent(_hJobEvent);
}
void ChatServer::OnRecv(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 오브젝트풀에서 Job 할당
	//--------------------------------------------------------------------
	CHAT_JOB* job = _jobPool.Alloc();
	job->type = JOB_TYPE_MESSAGE_RECV;
	job->sessionID = sessionID;
	job->packet = packet;

	//--------------------------------------------------------------------
	// Job 큐잉
	//--------------------------------------------------------------------
	packet->IncrementRefCount();
	_jobQ.Enqueue(job);

	//--------------------------------------------------------------------
	// UpdateThread 에게 Job 이벤트 알림
	//--------------------------------------------------------------------
	SetEvent(_hJobEvent);
}
void ChatServer::OnClientLeave(DWORD64 sessionID)
{
	//--------------------------------------------------------------------
	// 오브젝트풀에서 Job 할당
	//--------------------------------------------------------------------
	CHAT_JOB* job = _jobPool.Alloc();
	job->type = JOB_TYPE_CLIENT_LEAVE;
	job->sessionID = sessionID;

	//--------------------------------------------------------------------
	// Job 큐잉
	//--------------------------------------------------------------------
	_jobQ.Enqueue(job);

	//--------------------------------------------------------------------
	// UpdateThread 에게 Job 이벤트 알림
	//--------------------------------------------------------------------
	SetEvent(_hJobEvent);
}
void ChatServer::OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------
	// Network IO Error 로깅
	//--------------------------------------------------------------------
	Logger::WriteLog(L"Chat"
		, LOG_LEVEL_ERROR
		, L"func: %s, line: %d, error: %d, wParam: %llu, lParam: %llu"
		, funcname, linenum, errcode, wParam, lParam);

	//--------------------------------------------------------------------
	// Fatal Error 일 경우 크래시와 함께 메모리 덤프를 남긴다.
	//--------------------------------------------------------------------
	if (errcode >= NET_FATAL_INVALID_SIZE)
		CrashDump::Crash();
}
bool ChatServer::Initial()
{
	_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (_hExitEvent == NULL)
		return false;

	_hJobEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (_hJobEvent == NULL)
	{
		CloseHandle(_hExitEvent);
		return false;
	}

	//--------------------------------------------------------------------
	// Redis Connect
	//--------------------------------------------------------------------
	std::string redisIP;
	UnicodeToString(ServerConfig::GetRedisIP(), redisIP);
	_memorydb.connect(redisIP, ServerConfig::GetRedisPort());

	//--------------------------------------------------------------------
	// Thread Begin
	//--------------------------------------------------------------------
	_updateThread = std::thread(&ChatServer::UpdateThread, this);
	_managementThread = std::thread(&ChatServer::ManagementThread, this);
	return true;
}
void ChatServer::Release()
{
	//--------------------------------------------------------------------
	// Thread End
	//--------------------------------------------------------------------
	SetEvent(_hExitEvent);
	_updateThread.join();
	_managementThread.join();

	//--------------------------------------------------------------------
	// Redis Disconnect
	//--------------------------------------------------------------------
	_memorydb.disconnect();

	USER* user;
	for (auto iter = _userMap.begin(); iter != _userMap.end();)
	{
		user = iter->second;
		_userPool.Free(user);
		iter = _userMap.erase(iter);
	}

	PLAYER* player;
	for (auto iter = _playerMap.begin(); iter != _playerMap.end();)
	{
		player = iter->second;
		_playerPool.Free(player);
		iter = _playerMap.erase(iter);
	}

	CHAT_JOB* job;
	while (_jobQ.size() > 0)
	{
		_jobQ.Dequeue(job);
		if (job->type == JOB_TYPE_PACKET_RECV)
			NetPacket::Free(job->packet);
		_jobPool.Free(job);
	}

	CloseHandle(_hExitEvent);
	CloseHandle(_hJobEvent);
}
void ChatServer::UpdateThread()
{
	HANDLE hHandle[2] = { _hJobEvent, _hExitEvent };
	DWORD ret;
	while (1)
	{
		ret = WaitForMultipleObjects(2, hHandle, FALSE, INFINITE);
		if ((ret - WAIT_OBJECT_0) != 0)
			break;

		CHAT_JOB* job;
		while (_jobQ.size() > 0)
		{
			//--------------------------------------------------------------------
			// Job 디큐잉
			//--------------------------------------------------------------------
			_jobQ.Dequeue(job);

			//--------------------------------------------------------------------
			// Job Type 에 따른 분기 처리
			//--------------------------------------------------------------------
			switch (job->type)
			{
			case JOB_TYPE_CLIENT_JOIN:
				JoinProc(job->sessionID);
				break;
			case JOB_TYPE_CLIENT_LEAVE:
				LeaveProc(job->sessionID);
				break;
			case JOB_TYPE_MESSAGE_RECV:
				RecvProc(job->sessionID, job->packet);
				break;
			case JOB_TYPE_LOGIN_SUCCESS:
				LoginProc(job->sessionID, true);
				break;
			case JOB_TYPE_LOGIN_FAIL:
				LoginProc(job->sessionID, false);
				break;
			default:
				break;
			}

			//--------------------------------------------------------------------
			// 오브젝트풀에 Job 반납
			//--------------------------------------------------------------------
			_jobPool.Free(job);

			_curUpdateTPS++;
		}
	}
}
void ChatServer::ManagementThread()
{
	DWORD ret;
	while (1)
	{
		ret = WaitForSingleObject(_hExitEvent, 1000);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			return;
		case WAIT_TIMEOUT:
			UpdateTPS();
			break;
		default:
			break;
		}
	}
}
void ChatServer::UpdateTPS()
{
	_oldUpdateTPS.exchange(_curUpdateTPS.exchange(0));
}
void ChatServer::JoinProc(DWORD64 sessionID)
{
	//--------------------------------------------------------------------
	// 현재 접속자 수를 확인하여 Join 여부 판단
	//--------------------------------------------------------------------
	if (_userMap.size() >= ServerConfig::GetUserMax())
	{
		Disconnect(sessionID);
		return;
	}

	//--------------------------------------------------------------------
	// 새로 연결된 세션에 대한 객체 할당
	//--------------------------------------------------------------------
	NewUser(sessionID);
}
void ChatServer::RecvProc(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 수신 받은 메시지 처리
	//--------------------------------------------------------------------
	WORD type;

	try
	{
		(*packet) >> type;

		if (!PacketProc(sessionID, packet, type))
			Disconnect(sessionID);
	}
	catch (NetException& ex)
	{
		Logger::WriteLog(L"Chat", LOG_LEVEL_ERROR, L"NetException: %d, sessionID: %p", ex.GetLastError(), sessionID);
		Disconnect(sessionID);
	}

	NetPacket::Free(packet);
}
void ChatServer::LeaveProc(DWORD64 sessionID)
{
	//--------------------------------------------------------------------
	// 연결 종료한 세션의 객체 제거
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	if (user->login)
		DeletePlayer(user->accountNo);
	DeleteUser(sessionID);
}
void ChatServer::LoginProc(DWORD64 sessionID, bool result)
{
	//--------------------------------------------------------------------
	// 로그인 완료 처리
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	if (user->login)
	{
		Disconnect(sessionID);
		return;
	}

	NewPlayer(user->sessionID, user->accountNo);
	user->login = true;

	//--------------------------------------------------------------------
	// 오브젝트풀에서 직렬화 버퍼 할당
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// 해당 유저에게 로그인 완료 메시지 보내기
	//--------------------------------------------------------------------
	Packet::MakeChatLogin(resPacket, result, user->accountNo);
	SendPacket(user->sessionID, resPacket);

	//--------------------------------------------------------------------
	// 오브젝트풀에 직렬화 버퍼 반납
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
}
void ChatServer::NewUser(DWORD64 sessionID)
{
	USER* user = _userPool.Alloc();
	user->sessionID = sessionID;
	user->login = false;
	
	_userMap.insert({ sessionID, user });
}
void ChatServer::DeleteUser(DWORD64 sessionID)
{
	USER* user;
	auto iter = _userMap.find(sessionID);
	if (iter != _userMap.end())
	{
		user = iter->second;
		_userMap.erase(iter);
		_userPool.Free(user);
	}
}
USER* ChatServer::FindUser(INT64 sessionID)
{
	USER* user;
	auto iter = _userMap.find(sessionID);
	if (iter != _userMap.end())
	{
		user = iter->second;
		return user;
	}
	return nullptr;
}
void ChatServer::NewPlayer(DWORD64 sessionID, INT64 accountNo)
{
	PLAYER* player = _playerPool.Alloc();
	player->sessionID = sessionID;
	player->accountNo = accountNo;
	player->sector.x = dfUNKNOWN_SECTOR;
	player->sector.y = dfUNKNOWN_SECTOR;

	_playerMap.insert({ accountNo, player });
}
void ChatServer::DeletePlayer(INT64 accountNo)
{
	auto iter = _playerMap.find(accountNo);
	PLAYER* player = iter->second;
	if (player->sector.x != dfUNKNOWN_SECTOR && player->sector.y != dfUNKNOWN_SECTOR)
		RemovePlayer_Sector(player);

	_playerMap.erase(iter);
	_playerPool.Free(player);
}
PLAYER* ChatServer::FindPlayer(INT64 accountNo)
{
	PLAYER* player;
	auto iter = _playerMap.find(accountNo);
	if (iter != _playerMap.end())
	{
		player = iter->second;
		return player;
	}
	return nullptr;
}
bool ChatServer::IsMovablePlayer(int sectorX, int sectorY)
{
	//--------------------------------------------------------------------
	// 섹터 좌표 이동 가능여부 확인
	//--------------------------------------------------------------------
	if (sectorX < dfSECTOR_MAX_X && sectorY < dfSECTOR_MAX_Y)
		return true;

	return false;
}
void ChatServer::AddPlayer_Sector(PLAYER* player, int sectorX, int sectorY)
{
	//--------------------------------------------------------------------
	// 섹터 리스트에 플레이어 추가
	//--------------------------------------------------------------------
	player->sector.x = sectorX;
	player->sector.y = sectorY;
	_sectorList[player->sector.y][player->sector.x].push_back(player);
}
void ChatServer::RemovePlayer_Sector(PLAYER* player)
{
	//--------------------------------------------------------------------
	// 섹터 리스트에서 플레이어 제거
	//--------------------------------------------------------------------
	_sectorList[player->sector.y][player->sector.x].remove(player);
}
void ChatServer::UpdatePlayer_Sector(PLAYER* player, int sectorX, int sectorY)
{
	if (player->sector.x != dfUNKNOWN_SECTOR && player->sector.y != dfUNKNOWN_SECTOR)
		RemovePlayer_Sector(player);

	AddPlayer_Sector(player, sectorX, sectorY);
}
void ChatServer::GetSectorAround(int sectorX, int sectorY, SECTOR_AROUND* sectorAround)
{
	//--------------------------------------------------------------------
	// 특정 좌표 기준으로 주변 영향권 섹터 얻기
	//--------------------------------------------------------------------
	sectorX--;
	sectorY--;

	sectorAround->count = 0;
	for (int y = 0; y < 3; y++)
	{
		if (sectorY + y < 0 || sectorY + y >= dfSECTOR_MAX_Y)
			continue;

		for (int x = 0; x < 3; x++)
		{
			if (sectorX + x < 0 || sectorX + x >= dfSECTOR_MAX_X)
				continue;

			sectorAround->around[sectorAround->count].x = sectorX + x;
			sectorAround->around[sectorAround->count].y = sectorY + y;
			sectorAround->count++;
		}
	}
}
void ChatServer::SendSectorOne(NetPacket* packet, int sectorX, int sectorY)
{
	//--------------------------------------------------------------------
	// 특정 섹터에 있는 유저들에게 메시지 보내기
	//--------------------------------------------------------------------
	std::list<PLAYER*> *sectorList = &_sectorList[sectorY][sectorX];
	for (auto iter = sectorList->begin(); iter != sectorList->end(); ++iter)
	{
		SendPacket((*iter)->sessionID, packet);
	}
}
void ChatServer::SendSectorAround(PLAYER* player, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 주변 영향권 섹터에 있는 유저들에게 메시지 보내기
	//--------------------------------------------------------------------
	SECTOR_AROUND sectorAround;
	GetSectorAround(player->sector.x, player->sector.y, &sectorAround);

	for (int i = 0; i < sectorAround.count; i++)
	{
		SendSectorOne(packet, sectorAround.around[i].x, sectorAround.around[i].y);
	}
}
bool ChatServer::PacketProc(DWORD64 sessionID, NetPacket* packet, WORD type)
{
	//--------------------------------------------------------------------
	// 수신 메시지 타입에 따른 분기 처리
	//--------------------------------------------------------------------
	switch (type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		return PacketProc_ChatLogin(sessionID, packet);
	case en_PACKET_CS_CHAT_RES_LOGIN:
		break;
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		return PacketProc_ChatSectorMove(sessionID, packet);
	case en_PACKET_CS_CHAT_RES_SECTOR_MOVE:
		break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		return PacketProc_ChatMessage(sessionID, packet);
	case en_PACKET_CS_CHAT_RES_MESSAGE:
		break;
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		return true;
	default:
		break;
	}
	return false;
}
bool ChatServer::PacketProc_ChatLogin(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 로그인 메시지 처리
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	INT64 accountNo;
	WCHAR id[20];
	WCHAR nickname[20];
	char sessionKey[64];

	(*packet) >> accountNo;
	if (packet->GetData((char*)&id, sizeof(id)) != sizeof(id))
		return false;

	if (packet->GetData((char*)&nickname, sizeof(nickname)) != sizeof(nickname))
		return false;

	if (packet->GetData(sessionKey, sizeof(sessionKey)) != sizeof(sessionKey))
		return false;

	//--------------------------------------------------------------------
	// 이미 로그인한 유저인지 검증
	//--------------------------------------------------------------------
	if (user->login)
		return false;

	user->accountNo = accountNo;
	wcscpy_s(user->id, sizeof(id) / 2, id);
	wcscpy_s(user->nickname, sizeof(nickname) / 2, nickname);

	//--------------------------------------------------------------------
	// Redis 에서 세션 토큰 얻기 (Key: AccountNo, Value: SessionKey)
	//--------------------------------------------------------------------
	std::string key(std::to_string(accountNo));
	std::string token(sessionKey, sizeof(sessionKey));
	_memorydb.get(key, [this, sessionID, sessionKey = std::move(token)](cpp_redis::reply& reply) {
		//--------------------------------------------------------------------
		// 오브젝트풀에서 Job 할당
		//--------------------------------------------------------------------
		CHAT_JOB* job = _jobPool.Alloc();
		job->type = (sessionKey == reply.as_string()) ? JOB_TYPE_LOGIN_SUCCESS : JOB_TYPE_LOGIN_FAIL;
		job->sessionID = sessionID;
		
		//--------------------------------------------------------------------
		// Job 큐잉
		//--------------------------------------------------------------------
		_jobQ.Enqueue(job);
		
		//--------------------------------------------------------------------
		// UpdateThread 에게 Job 이벤트 알림
		//--------------------------------------------------------------------
		SetEvent(_hJobEvent);
	});

	//--------------------------------------------------------------------
	// Redis 에서 세션 토큰 제거
	//--------------------------------------------------------------------
	std::vector<std::string> vectKey(1);
	vectKey.emplace_back(std::move(key));
	_memorydb.del(vectKey);

	_memorydb.commit();
	return true;
}
bool ChatServer::PacketProc_ChatSectorMove(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 섹터 이동 처리
	//--------------------------------------------------------------------
	INT64 accountNo;
	WORD sectorX;
	WORD sectorY;
	(*packet) >> accountNo >> sectorX >> sectorY;

	//--------------------------------------------------------------------
	// 로그인하지 않은 유저인지 검증
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	if (!user->login)
		return false;

	//--------------------------------------------------------------------
	// 계정 번호 검증
	//--------------------------------------------------------------------
	if (user->accountNo != accountNo)
		return false;

	//--------------------------------------------------------------------
	// 유저가 이동 요청한 좌표가 실제로 이동 가능한 좌표인지 확인
	//--------------------------------------------------------------------
	if (!IsMovablePlayer(sectorX, sectorY))
		return false;

	//--------------------------------------------------------------------
	// 섹터 이동 처리
	//--------------------------------------------------------------------
	PLAYER* player = FindPlayer(user->accountNo);
	if (player->sector.x != sectorX || player->sector.y != sectorY)
		UpdatePlayer_Sector(player, sectorX, sectorY);

	//--------------------------------------------------------------------
	// 오브젝트풀에서 직렬화 버퍼 할당
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// 해당 유저에게 섹터 이동 메시지 보내기
	//--------------------------------------------------------------------
	Packet::MakeChatSectorMove(resPacket
		, player->accountNo
		, player->sector.x
		, player->sector.y);
	SendPacket(player->sessionID, resPacket);

	//--------------------------------------------------------------------
	// 오브젝트풀에 직렬화 버퍼 반납
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
bool ChatServer::PacketProc_ChatMessage(DWORD64 sessionID, NetPacket* packet)
{
	//--------------------------------------------------------------------
	// 채팅 메시지 처리
	//--------------------------------------------------------------------
	INT64 accountNo;
	WORD messageLen;
	WCHAR message[256];
	(*packet) >> accountNo >> messageLen;

	if (messageLen > sizeof(message))
		return false;

	if (packet->GetData((char*)message, messageLen) != messageLen)
		return false;

	//--------------------------------------------------------------------
	// 로그인하지 않은 유저인지 검증
	//--------------------------------------------------------------------
	USER* user = FindUser(sessionID);
	if (!user->login)
		return false;

	//--------------------------------------------------------------------
	// 계정 번호 검증
	//--------------------------------------------------------------------
	if (user->accountNo != accountNo)
		return false;

	//--------------------------------------------------------------------
	// 오브젝트풀에서 직렬화 버퍼 할당
	//--------------------------------------------------------------------
	NetPacket* resPacket = NetPacket::Alloc();

	//--------------------------------------------------------------------
	// 주변 영향권 섹터의 유저들에게 채팅 메시지 보내기
	//--------------------------------------------------------------------
	Packet::MakeChatMessage(resPacket
		, user->accountNo
		, user->id
		, user->nickname
		, messageLen
		, message);

	PLAYER* player = FindPlayer(user->accountNo);
	SendSectorAround(player, resPacket);

	//--------------------------------------------------------------------
	// 오브젝트풀에 직렬화 버퍼 반납
	//--------------------------------------------------------------------
	NetPacket::Free(resPacket);
	return true;
}
