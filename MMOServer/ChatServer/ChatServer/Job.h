#pragma once
#include "../../Lib/Network/include/NetPacket.h"

enum JOB_TYPE
{
	//------------------------------------------------------
	// Chat Job
	//------------------------------------------------------
	JOB_TYPE_CLIENT_JOIN = 0,
	JOB_TYPE_CLIENT_LEAVE,
	JOB_TYPE_MESSAGE_RECV,
	JOB_TYPE_LOGIN_SUCCESS,
	JOB_TYPE_LOGIN_FAIL,

	//------------------------------------------------------
	// Auth Job
	//------------------------------------------------------
	JOB_TYPE_LOGIN_AUTH = 1000,
};

struct CHAT_JOB
{
	WORD type;
	DWORD64 sessionID;
	Jay::NetPacket* packet;
};

struct AUTH_JOB
{
	WORD type;
	DWORD64 sessionID;
	INT64 accountNo;
	CHAR token[65];
};
