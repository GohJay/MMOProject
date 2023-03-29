#include "stdafx.h"
#include "Packet.h"
#include "../../Common/CommonProtocol.h"

void Packet::MakeGameLogin(Jay::NetPacket* packet, BYTE status, INT64 accountNo)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_LOGIN;
	(*packet) << status;
	(*packet) << accountNo;
}
void Packet::MakeGameEcho(Jay::NetPacket* packet, INT64 accountNo, LONGLONG sendTick)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_ECHO;
	(*packet) << accountNo;
	(*packet) << sendTick;
}
void Packet::MakeMonitorLogin(Jay::NetPacket* packet, int serverNo)
{
	(*packet) << (WORD)en_PACKET_SS_MONITOR_LOGIN;
	(*packet) << serverNo;
}
void Packet::MakeMonitorDataUpdate(Jay::NetPacket* packet, BYTE dataType, int dataValue, int timeStamp)
{
	(*packet) << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
	(*packet) << dataType;
	(*packet) << dataValue;
	(*packet) << timeStamp;
}
