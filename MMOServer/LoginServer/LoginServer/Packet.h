#pragma once
#include "../../Lib/Network/include/NetPacket.h"

class Packet
{
public:
	static void MakeLogin(Jay::NetPacket* packet, INT64 accountNo, BYTE status, WCHAR* userID, WCHAR* nickname, WCHAR* gameServerIP, USHORT gameServerPort, WCHAR* chatServerIP, USHORT chatServerPort);
};
