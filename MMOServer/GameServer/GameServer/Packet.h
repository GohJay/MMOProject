#pragma once
#include "../../Lib/Network/include/NetPacket.h"

class Packet
{
public:
	static void MakeGameLogin(Jay::NetPacket* packet, BYTE status, INT64 accountNo);
	static void MakeGameEcho(Jay::NetPacket* packet, INT64 accountNo, LONGLONG sendTick);
	static void MakeMonitorLogin(Jay::NetPacket* packet, int serverNo);
	static void MakeMonitorDataUpdate(Jay::NetPacket* packet, BYTE dataType, int dataValue, int timeStamp);
};
