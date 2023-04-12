#pragma once
#include "../../Lib/Network/include/NetPacket.h"

class Packet
{
public:
	static void MakeGameLogin(Jay::NetPacket* packet, BYTE status, INT64 accountNo);
	static void MakeCharacterSelect(Jay::NetPacket* packet, BYTE characterType);
	static void MakeCreateMyCharacter(Jay::NetPacket* packet, INT64 clientID, BYTE characterType, WCHAR* nickname, float posX, float posY, USHORT rotation, int cristal, int hp, INT64 exp, USHORT level);
	static void MakeCreateOtherCharacter(Jay::NetPacket* packet, INT64 clientID, BYTE characterType, WCHAR* nickname, float posX, float posY, USHORT rotation, USHORT level, BYTE respawn, BYTE sit, BYTE die);
	static void MakeCreateMonster(Jay::NetPacket* packet, INT64 clientID, float posX, float posY, USHORT rotation, BYTE respawn);
	static void MakeDeleteObject(Jay::NetPacket* packet, INT64 clientID);
	static void MakeMoveCharacterStart(Jay::NetPacket* packet, INT64 clientID, float posX, float posY, USHORT rotation, BYTE vKey, BYTE hKey);
	static void MakeMoveCharacterStop(Jay::NetPacket* packet, INT64 clientID, float posX, float posY, USHORT rotation);
	static void MakeMoveMonster(Jay::NetPacket* packet, INT64 clientID, float posX, float posY, USHORT rotation);
	static void MakeAttack1(Jay::NetPacket* packet, INT64 clientID);
	static void MakeAttack2(Jay::NetPacket* packet, INT64 clientID);
	static void MakeMonsterAttack(Jay::NetPacket* packet, INT64 clientID);
	static void MakeDamage(Jay::NetPacket* packet, INT64 attackID, INT64 damageID, int damage);
	static void MakeMonsterDie(Jay::NetPacket* packet, INT64 clientID);
	static void MakeCreateCristal(Jay::NetPacket* packet, INT64 clientID, BYTE cristalType, float posX, float posY);
	static void MakePickCristal(Jay::NetPacket* packet, INT64 clientID);
	static void MakeSitCharacter(Jay::NetPacket* packet, INT64 clientID);
	static void MakeGetCristal(Jay::NetPacket* packet, INT64 clientID, INT64 cristalClientID, int amountCristal);
	static void MakeSyncHP(Jay::NetPacket* packet, int hp);
	static void MakeCharacterDie(Jay::NetPacket* packet, INT64 clientID, int minusCristal);
	static void MakeRestart(Jay::NetPacket* packet);
	static void MakeGameEcho(Jay::NetPacket* packet, INT64 accountNo, LONGLONG sendTick);
	static void MakeMonitorLogin(Jay::NetPacket* packet, int serverNo);
	static void MakeMonitorDataUpdate(Jay::NetPacket* packet, BYTE dataType, int dataValue, int timeStamp);
};
