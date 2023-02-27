#include "stdafx.h"
#include "Packet.h"
#include "../../Common/CommonProtocol.h"

void Packet::MakeLogin(Jay::NetPacket* packet, INT64 accountNo, BYTE status, WCHAR* userID, WCHAR* nickname, WCHAR* gameServerIP, USHORT gameServerPort, WCHAR* chatServerIP, USHORT chatServerPort)
{
	(*packet) << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN;
	(*packet) << accountNo;
	(*packet) << status;
	packet->PutData((char*)userID, 20 * 2);
	packet->PutData((char*)nickname, 20 * 2);
	packet->PutData((char*)gameServerIP, 16 * 2);
	(*packet) << gameServerPort;
	packet->PutData((char*)chatServerIP, 16 * 2);
	(*packet) << chatServerPort;
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

