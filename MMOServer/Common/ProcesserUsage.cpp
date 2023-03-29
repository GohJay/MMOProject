#include "ProcesserUsage.h"
#include <strsafe.h>
#pragma comment(lib,"Pdh.lib")

using namespace Jay;

ProcesserUsage::ProcesserUsage()
{
	// PDH 쿼리 핸들 열기
	PdhOpenQuery(NULL, NULL, &_pdhQuery);

	// PDH 리소스 카운터 생성
	InitPDHCounter();

	// 리소스 값 초기화
	_pdh_Value_Network_RecvBytes = 0;
	_pdh_Value_Network_SendBytes = 0;
	_pdh_Value_NPPool_UseBytes = 0;
	_pdh_Value_Memory_FreeMBytes = 0;
	_sys_CPU_UseTotal = 0;
	_sys_CPU_UseUser = 0;
	_sys_CPU_UseKernel = 0;
	_sys_CPU_LastIdle.QuadPart = 0;
	_sys_CPU_LastUser.QuadPart = 0;
	_sys_CPU_LastKernel.QuadPart = 0;
}
ProcesserUsage::~ProcesserUsage()
{
	// PDH 쿼리 핸들 닫기
	PdhCloseQuery(_pdhQuery);
}
void ProcesserUsage::Update()
{
	// PDH 리소스 값 갱신
	UpdatePDHValue();

	// System 리소스 값 갱신
	UpdateSystemValue();
}
float ProcesserUsage::GetUseCPUTotalTime()
{
	return (float)_sys_CPU_UseTotal;
}
float ProcesserUsage::GetUseCPUUserTime()
{
	return (float)_sys_CPU_UseUser;
}
float ProcesserUsage::GetUseCPUKernelTime()
{
	return (float)_sys_CPU_UseKernel;
}
float ProcesserUsage::GetUseNonpagedPoolMBytes()
{
	return (float)(_pdh_Value_NPPool_UseBytes / (1024 * 1024));
}
float ProcesserUsage::GetFreeMemoryMBytes()
{
	return (float)_pdh_Value_Memory_FreeMBytes;
}
float ProcesserUsage::GetNetworkRecvKBytes()
{
	return (float)(_pdh_Value_Network_RecvBytes / 1024);
}
float ProcesserUsage::GetNetworkSendKBytes()
{
	return (float)(_pdh_Value_Network_SendBytes / 1024);
}
void ProcesserUsage::AddEthernetCounter()
{
	int iCnt = 0;
	bool bErr = false;
	WCHAR* szCur = NULL;
	WCHAR* szCounters = NULL;
	WCHAR* szInterfaces = NULL;
	DWORD dwCounterSize = 0;
	DWORD dwInterfaceSize = 0;
	WCHAR szQuery[1024];

	//---------------------------------------------------------------------------------------
	// PdhEnumObjectItems 을 통해서 "NetworkInterface" 항목에서 얻을 수 있는
	// 측정항목(Counters) / 인터페이스 항목(Interfaces) 를 얻음. 그런데 그 개수나 길이를 모르기 때문에
	// 먼저 버퍼의 길이를 알기 위해서 Out Buffer 인자들을 NULL 포인터로 넣어서 사이즈만 확인.
	//---------------------------------------------------------------------------------------
	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);

	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];

	//---------------------------------------------------------------------------------------
	// 버퍼의 동적할당 후 다시 호출!
	//
	// szCounters 와 szInterfaces 버퍼에는 여러개의 문자열이 들어온다. 2차원 배열도 아니고,
	// 그냥 NULL 포인터로 끝나는 문자열들이 dwCounterSize, dwInterfaceSize 길이만큼 줄줄이 들어있음.
	// 이를 문자열 단위로 끊어서 개수를 확인 해야 함. ex) aaa\0bbb\0ccc\0ddd\0
	//---------------------------------------------------------------------------------------
	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
		return;
	}

	//---------------------------------------------------------------------------------------
	// szInterfaces 에서 문자열 단위로 끊으면서 이더넷 이름과 사용량을 복사한다.
	//---------------------------------------------------------------------------------------
	szCur = szInterfaces;
	for (iCnt = 0; iCnt < MAX_PDH_ETHERNET; iCnt++)
	{
		if (*szCur == L'\0')
		{
			_ethernet[iCnt].used = false;
			continue;
		}

		_ethernet[iCnt].used = true;
		wcscpy_s(_ethernet[iCnt].name, szCur);

		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
		PdhAddCounter(_pdhQuery, szQuery, NULL, &_ethernet[iCnt].pdh_Counter_Network_RecvBytes);

		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
		PdhAddCounter(_pdhQuery, szQuery, NULL, &_ethernet[iCnt].pdh_Counter_Network_SendBytes);

		szCur += wcslen(szCur) + 1;
	}

	delete[] szCounters;
	delete[] szInterfaces;
}
void ProcesserUsage::InitPDHCounter()
{
	AddEthernetCounter();
	PdhAddCounter(_pdhQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_pdh_Counter_NPPool_UseBytes);
	PdhAddCounter(_pdhQuery, L"\\Memory\\Available MBytes", NULL, &_pdh_Counter_Memory_FreeMBytes);
}
void ProcesserUsage::UpdatePDHValue()
{
	//---------------------------------------------------------------------------
	// 지정된 PDH 리소스 사용률을 갱신한다.
	//---------------------------------------------------------------------------
	PDH_FMT_COUNTERVALUE pdhCounterValue;
	double pdh_Value_Network_RecvBytes = 0;
	double pdh_Value_Network_SendBytes = 0;

	//---------------------------------------------------------------------------
	// PDH 리소스 카운터 갱신
	//---------------------------------------------------------------------------
	PdhCollectQueryData(_pdhQuery);

	//---------------------------------------------------------------------------------------
	// Network 사용량 추출
	//---------------------------------------------------------------------------------------
	for (int iCnt = 0; iCnt < MAX_PDH_ETHERNET; iCnt++)
	{
		if (_ethernet[iCnt].used)
		{
			if (PdhGetFormattedCounterValue(_ethernet[iCnt].pdh_Counter_Network_RecvBytes, PDH_FMT_DOUBLE, NULL, &pdhCounterValue) == ERROR_SUCCESS)
				pdh_Value_Network_RecvBytes += pdhCounterValue.doubleValue;

			if (PdhGetFormattedCounterValue(_ethernet[iCnt].pdh_Counter_Network_SendBytes, PDH_FMT_DOUBLE, NULL, &pdhCounterValue) == ERROR_SUCCESS)
				pdh_Value_Network_SendBytes += pdhCounterValue.doubleValue;
		}
	}
	_pdh_Value_Network_RecvBytes = pdh_Value_Network_RecvBytes;
	_pdh_Value_Network_SendBytes = pdh_Value_Network_SendBytes;

	//---------------------------------------------------------------------------------------
	// Non-paged Pool 사용량 추출
	//---------------------------------------------------------------------------------------
	if (PdhGetFormattedCounterValue(_pdh_Counter_NPPool_UseBytes, PDH_FMT_DOUBLE, NULL, &pdhCounterValue) == ERROR_SUCCESS)
		_pdh_Value_NPPool_UseBytes = pdhCounterValue.doubleValue;
	else
		_pdh_Value_NPPool_UseBytes = 0.0f;

	//---------------------------------------------------------------------------------------
	// Memory 여유량 추출
	//---------------------------------------------------------------------------------------
	if (PdhGetFormattedCounterValue(_pdh_Counter_Memory_FreeMBytes, PDH_FMT_DOUBLE, NULL, &pdhCounterValue) == ERROR_SUCCESS)
		_pdh_Value_Memory_FreeMBytes = pdhCounterValue.doubleValue;
	else
		_pdh_Value_Memory_FreeMBytes = 0.0f;
}
void ProcesserUsage::UpdateSystemValue()
{
	//---------------------------------------------------------------------------
	// 프로세서 사용률을 갱신한다.
	// FILETIME 구조체는 100 나노세컨드 단위의 시간 단위를 표현하는 구조체이다.
	//---------------------------------------------------------------------------
	ULARGE_INTEGER idle;
	ULARGE_INTEGER user;
	ULARGE_INTEGER kernel;
	ULONGLONG total;
	ULONGLONG idleDiff;
	ULONGLONG userDiff;
	ULONGLONG kernelDiff;

	//---------------------------------------------------------------------------
	// 시스템 사용 시간을 구한다.
	// 아이들 타임 / 커널 사용 타임 (아이들포함) / 유저 사용 타임
	//---------------------------------------------------------------------------
	if (!GetSystemTimes((PFILETIME)&idle, (PFILETIME)&kernel, (PFILETIME)&user))
		return;

	// 커널 타임에는 아이들 타임이 포함된다.
	kernelDiff = kernel.QuadPart - _sys_CPU_LastKernel.QuadPart;
	userDiff = user.QuadPart - _sys_CPU_LastUser.QuadPart;
	idleDiff = idle.QuadPart - _sys_CPU_LastIdle.QuadPart;
	total = kernelDiff + userDiff;

	if (total == 0)
	{
		_sys_CPU_UseTotal = 0.0f;
		_sys_CPU_UseUser = 0.0f;
		_sys_CPU_UseKernel = 0.0f;
	}
	else
	{
		// 커널 타임에 아이들 타임이 있으므로 빼서 계산.
		_sys_CPU_UseTotal = (double)(total - idleDiff) / total * 100.0f;
		_sys_CPU_UseUser = (double)userDiff / total * 100.0f;
		_sys_CPU_UseKernel = (double)(kernelDiff - idleDiff) / total * 100.0f;
	}

	_sys_CPU_LastKernel = kernel;
	_sys_CPU_LastUser = user;
	_sys_CPU_LastIdle = idle;
}
