#include "stdafx.h"
#include "Packet.h"
#include "../../Common/CommonProtocol.h"

void Packet::MakeGameLogin(Jay::NetPacket* packet, BYTE status, INT64 accountNo)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_LOGIN;
	(*packet) << status;
	(*packet) << accountNo;
}
void Packet::MakeCharacterSelect(Jay::NetPacket* packet, BYTE characterType)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT;
	(*packet) << characterType;
}
void Packet::MakeCreateMyCharacter(Jay::NetPacket* packet, INT64 clientID, BYTE characterType, WCHAR* nickname, float fieldX, float fieldY, USHORT rotation, int cristal, int hp, INT64 exp, USHORT level)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER;
	(*packet) << clientID;
	(*packet) << characterType;
	packet->PutData((char*)nickname, 20 * sizeof(WCHAR));
	(*packet) << fieldX;
	(*packet) << fieldY;
	(*packet) << rotation;
	(*packet) << cristal;
	(*packet) << hp;
	(*packet) << exp;
	(*packet) << level;
}
void Packet::MakeCreateOtherCharacter(Jay::NetPacket* packet, INT64 clientID, BYTE characterType, WCHAR* nickname, float fieldX, float fieldY, USHORT rotation, USHORT level, BYTE respawn, BYTE sit, BYTE die)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER;
	(*packet) << clientID;
	(*packet) << characterType;
	packet->PutData((char*)nickname, 20 * sizeof(WCHAR));
	(*packet) << fieldX;
	(*packet) << fieldY;
	(*packet) << rotation;
	(*packet) << level;
	(*packet) << respawn;
	(*packet) << sit;
	(*packet) << die;
}
void Packet::MakeCreateMonster(Jay::NetPacket* packet, INT64 clientID, float fieldX, float fieldY, USHORT rotation, BYTE respawn)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER;
	(*packet) << clientID;
	(*packet) << fieldX;
	(*packet) << fieldY;
	(*packet) << rotation;
	(*packet) << respawn;
}
void Packet::MakeDeleteObject(Jay::NetPacket* packet, INT64 clientID)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_REMOVE_OBJECT;
	(*packet) << clientID;
}
void Packet::MakeMoveCharacterStart(Jay::NetPacket* packet, INT64 clientID, float fieldX, float fieldY, USHORT rotation, BYTE vKey, BYTE hKey)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_MOVE_CHARACTER;
	(*packet) << clientID;
	(*packet) << fieldX;
	(*packet) << fieldY;
	(*packet) << rotation;
	(*packet) << vKey;
	(*packet) << hKey;
}
void Packet::MakeMoveCharacterStop(Jay::NetPacket* packet, INT64 clientID, float fieldX, float fieldY, USHORT rotation)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_STOP_CHARACTER;
	(*packet) << clientID;
	(*packet) << fieldX;
	(*packet) << fieldY;
	(*packet) << rotation;
}
void Packet::MakeMoveMonster(Jay::NetPacket* packet, INT64 clientID, float fieldX, float fieldY, USHORT rotation)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_MOVE_MONSTER;
	(*packet) << clientID;
	(*packet) << fieldX;
	(*packet) << fieldY;
	(*packet) << rotation;
}
void Packet::MakeAttack1(Jay::NetPacket* packet, INT64 clientID)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_ATTACK1;
	(*packet) << clientID;
}
void Packet::MakeAttack2(Jay::NetPacket* packet, INT64 clientID)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_ATTACK2;
	(*packet) << clientID;
}
void Packet::MakeMonsterAttack(Jay::NetPacket* packet, INT64 clientID)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_MONSTER_ATTACK;
	(*packet) << clientID;
}
void Packet::MakeDamage(Jay::NetPacket* packet, INT64 attackID, INT64 damageID, int damage)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_DAMAGE;
	(*packet) << attackID;
	(*packet) << damageID;
	(*packet) << damage;
}
void Packet::MakeMonsterDie(Jay::NetPacket* packet, INT64 clientID)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_MONSTER_DIE;
	(*packet) << clientID;
}
void Packet::MakeCreateCristal(Jay::NetPacket* packet, INT64 clientID, BYTE cristalType, float fieldX, float fieldY)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_CREATE_CRISTAL;
	(*packet) << clientID;
	(*packet) << cristalType;
	(*packet) << fieldX;
	(*packet) << fieldY;
}
void Packet::MakePickCristal(Jay::NetPacket* packet, INT64 clientID)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_PICK;
	(*packet) << clientID;
}
void Packet::MakeSitCharacter(Jay::NetPacket* packet, INT64 clientID)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_SIT;
	(*packet) << clientID;
}
void Packet::MakeGetCristal(Jay::NetPacket* packet, INT64 clientID, INT64 cristalClientID, int amountCristal)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_PICK_CRISTAL;
	(*packet) << clientID;
	(*packet) << cristalClientID;
	(*packet) << amountCristal;
}
void Packet::MakeCorrectHP(Jay::NetPacket* packet, int hp)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_PLAYER_HP;
	(*packet) << hp;
}
void Packet::MakeCharacterDie(Jay::NetPacket* packet, INT64 clientID, int minusCristal)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_PLAYER_DIE;
	(*packet) << clientID;
	(*packet) << minusCristal;
}
void Packet::MakeRestart(Jay::NetPacket* packet)
{
	(*packet) << (WORD)en_PACKET_CS_GAME_RES_PLAYER_RESTART;
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
