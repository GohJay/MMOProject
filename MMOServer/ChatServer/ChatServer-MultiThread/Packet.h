#pragma once
#include "../../Lib/Network/include/NetPacket.h"

class Packet
{
public:
	static void MakeChatLogin(Jay::NetPacket* packet, BYTE status, INT64 accountNo);
	static void MakeChatSectorMove(Jay::NetPacket* packet, INT64 accountNo, WORD sectorX, WORD sectorY);
	static void MakeChatMessage(Jay::NetPacket* packet, INT64 accountNo, WCHAR* id, WCHAR* nickname, WORD messageLen, WCHAR* message);
	static void MakeMonitorLogin(Jay::NetPacket* packet, int serverNo);
	static void MakeMonitorDataUpdate(Jay::NetPacket* packet, BYTE dataType, int dataValue, int timeStamp);
};
