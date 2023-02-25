#pragma once
#include "../../Lib/Network/include/NetPacket.h"

class Packet
{
public:
	static void MakeLogin(Jay::NetPacket* packet, BYTE status);
	static void MakeDataUpdate(Jay::NetPacket* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp);
};
