#pragma once
#include "../../Lib/Network/include/NetPacket.h"

enum CHAT_JOB_TYPE
{
	JOB_TYPE_CLIENT_JOIN = 0,
	JOB_TYPE_CLIENT_LEAVE,
	JOB_TYPE_MESSAGE_RECV,
	JOB_TYPE_LOGIN_SUCCESS,
	JOB_TYPE_LOGIN_FAIL,
};

struct CHAT_JOB
{
	CHAT_JOB_TYPE type;
	DWORD64 sessionID;
	Jay::NetPacket* packet;
};
