#include "ProcesserUsage.h"
#include <strsafe.h>
#pragma comment(lib,"Pdh.lib")

using namespace Jay;

ProcesserUsage::ProcesserUsage()
{
	// PDH ���� �ڵ� ����
	PdhOpenQuery(NULL, NULL, &_pdhQuery);

	// PDH ���ҽ� ī���� ����
	InitPDHCounter();

	// ���ҽ� �� �ʱ�ȭ
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
	// PDH ���� �ڵ� �ݱ�
	PdhCloseQuery(_pdhQuery);
}
void ProcesserUsage::Update()
{
	// PDH ���ҽ� �� ����
	UpdatePDHValue();

	// System ���ҽ� �� ����
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
	// PdhEnumObjectItems �� ���ؼ� "NetworkInterface" �׸񿡼� ���� �� �ִ�
	// �����׸�(Counters) / �������̽� �׸�(Interfaces) �� ����. �׷��� �� ������ ���̸� �𸣱� ������
	// ���� ������ ���̸� �˱� ���ؼ� Out Buffer ���ڵ��� NULL �����ͷ� �־ ����� Ȯ��.
	//---------------------------------------------------------------------------------------
	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);

	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];

	//---------------------------------------------------------------------------------------
	// ������ �����Ҵ� �� �ٽ� ȣ��!
	//
	// szCounters �� szInterfaces ���ۿ��� �������� ���ڿ��� ���´�. 2���� �迭�� �ƴϰ�,
	// �׳� NULL �����ͷ� ������ ���ڿ����� dwCounterSize, dwInterfaceSize ���̸�ŭ ������ �������.
	// �̸� ���ڿ� ������ ��� ������ Ȯ�� �ؾ� ��. ex) aaa\0bbb\0ccc\0ddd\0
	//---------------------------------------------------------------------------------------
	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
		return;
	}

	//---------------------------------------------------------------------------------------
	// szInterfaces ���� ���ڿ� ������ �����鼭 �̴��� �̸��� ��뷮�� �����Ѵ�.
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
	// ������ PDH ���ҽ� ������ �����Ѵ�.
	//---------------------------------------------------------------------------
	PDH_FMT_COUNTERVALUE pdhCounterValue;
	double pdh_Value_Network_RecvBytes = 0;
	double pdh_Value_Network_SendBytes = 0;

	//---------------------------------------------------------------------------
	// PDH ���ҽ� ī���� ����
	//---------------------------------------------------------------------------
	PdhCollectQueryData(_pdhQuery);

	//---------------------------------------------------------------------------------------
	// Network ��뷮 ����
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
	// Non-paged Pool ��뷮 ����
	//---------------------------------------------------------------------------------------
	if (PdhGetFormattedCounterValue(_pdh_Counter_NPPool_UseBytes, PDH_FMT_DOUBLE, NULL, &pdhCounterValue) == ERROR_SUCCESS)
		_pdh_Value_NPPool_UseBytes = pdhCounterValue.doubleValue;
	else
		_pdh_Value_NPPool_UseBytes = 0.0f;

	//---------------------------------------------------------------------------------------
	// Memory ������ ����
	//---------------------------------------------------------------------------------------
	if (PdhGetFormattedCounterValue(_pdh_Counter_Memory_FreeMBytes, PDH_FMT_DOUBLE, NULL, &pdhCounterValue) == ERROR_SUCCESS)
		_pdh_Value_Memory_FreeMBytes = pdhCounterValue.doubleValue;
	else
		_pdh_Value_Memory_FreeMBytes = 0.0f;
}
void ProcesserUsage::UpdateSystemValue()
{
	//---------------------------------------------------------------------------
	// ���μ��� ������ �����Ѵ�.
	// FILETIME ����ü�� 100 ���뼼���� ������ �ð� ������ ǥ���ϴ� ����ü�̴�.
	//---------------------------------------------------------------------------
	ULARGE_INTEGER idle;
	ULARGE_INTEGER user;
	ULARGE_INTEGER kernel;
	ULONGLONG total;
	ULONGLONG idleDiff;
	ULONGLONG userDiff;
	ULONGLONG kernelDiff;

	//---------------------------------------------------------------------------
	// �ý��� ��� �ð��� ���Ѵ�.
	// ���̵� Ÿ�� / Ŀ�� ��� Ÿ�� (���̵�����) / ���� ��� Ÿ��
	//---------------------------------------------------------------------------
	if (!GetSystemTimes((PFILETIME)&idle, (PFILETIME)&kernel, (PFILETIME)&user))
		return;

	// Ŀ�� Ÿ�ӿ��� ���̵� Ÿ���� ���Եȴ�.
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
		// Ŀ�� Ÿ�ӿ� ���̵� Ÿ���� �����Ƿ� ���� ���.
		_sys_CPU_UseTotal = (double)(total - idleDiff) / total * 100.0f;
		_sys_CPU_UseUser = (double)userDiff / total * 100.0f;
		_sys_CPU_UseKernel = (double)(kernelDiff - idleDiff) / total * 100.0f;
	}

	_sys_CPU_LastKernel = kernel;
	_sys_CPU_LastUser = user;
	_sys_CPU_LastIdle = idle;
}
