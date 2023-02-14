#pragma once
#include "../../Lib/Network/include/NetPacket.h"

class Packet
{
public:
	static void MakeChatLogin(Jay::NetPacket* packet, BYTE status, INT64 accountNo);
	static void MakeChatSectorMove(Jay::NetPacket* packet, INT64 accountNo, WORD sectorX, WORD sectorY);
	static void MakeChatMessage(Jay::NetPacket* packet, INT64 accountNo, WCHAR* id, WCHAR* nickname, WORD messageLen, WCHAR* message);
};
