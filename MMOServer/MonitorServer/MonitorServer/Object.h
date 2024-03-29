#pragma once
#include <utility>

struct DATA
{
	DATA(BYTE type) : dataType(type)
	{
	}

	BYTE dataType;
	int totalData;
	int iMin;
	int iMax;
	int iCall;
	DWORD lastLogTime;
};

struct DATA_FINDER
{
	DATA_FINDER(BYTE type) : dataType(type)
	{
	}
	bool operator() (const std::pair<int, DATA*>& range) const
	{
		if (range.second->dataType == dataType)
			return true;

		return false;
	}

	BYTE dataType;
};
