#include "stdafx.h"
#include "Packet.h"
#include "../../Common/CommonProtocol.h"

void Packet::MakeLogin(Jay::NetPacket* packet, BYTE status)
{
	(*packet) << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
	(*packet) << status;
}
void Packet::MakeDataUpdate(Jay::NetPacket* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
{
	(*packet) << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	(*packet) << serverNo;
	(*packet) << dataType;
	(*packet) << dataValue;
	(*packet) << timeStamp;
}
