#pragma once
#include "../../Lib/Network/include/NetServerEx.h"

class GameServer : public Jay::NetServerEx
{
public:
	GameServer();
	~GameServer();
private:
	bool OnConnectionRequest(const wchar_t* ipaddress, int port) override;
	void OnError(int errcode, const wchar_t* funcname, int linenum, WPARAM wParam, LPARAM lParam) override;
};

enum CONTENT_ID
{
	CONTENT_ID_AUTH = 0,
	CONTENT_ID_GAME = 1,
};

enum CONTENT_FRAME_INTERVAL
{
	FRAME_INTERVAL_AUTH = 10,
	FRAME_INTERVAL_GAME = 20
};
