#include "stdafx.h"
#include "Packet.h"
#include "../Common/CommonProtocol.h"

void Packet::MakeChatLogin(Jay::NetPacket* packet, BYTE status, INT64 accountNo)
{
	(*packet) << (WORD)en_PACKET_CS_CHAT_RES_LOGIN;
	(*packet) << status;
	(*packet) << accountNo;
}
void Packet::MakeChatSectorMove(Jay::NetPacket* packet, INT64 accountNo, WORD sectorX, WORD sectorY)
{
	(*packet) << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	(*packet) << accountNo;
	(*packet) << sectorX;
	(*packet) << sectorY;
}
void Packet::MakeChatMessage(Jay::NetPacket* packet, INT64 accountNo, WCHAR* id, WCHAR* nickname, WORD messageLen, WCHAR* message)
{
	(*packet) << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE;
	(*packet) << accountNo;
	packet->PutData((char*)id, 20 * 2);
	packet->PutData((char*)nickname, 20 * 2);
	(*packet) << messageLen;
	packet->PutData((char*)message, messageLen);
}
