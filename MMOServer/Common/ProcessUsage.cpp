#include "ProcessUsage.h"
#include "FileUtil.h"
#include <strsafe.h>
#pragma comment(lib,"Pdh.lib")

using namespace Jay;

ProcessUsage::ProcessUsage(HANDLE hProcess)
{
	if (hProcess == INVALID_HANDLE_VALUE)
		hProcess = GetCurrentProcess();

	_hProcess = hProcess;
	GetModuleName(_hProcess, _processName);

	// PDH ���� �ڵ� ����
	PdhOpenQuery(NULL, NULL, &_pdhQuery);

	// PDH ���ҽ� ī���� ����
	InitPDHCounter();

	// System ���ҽ� ���� ���
	InitSystemInfo();

	// ���ҽ� �� �ʱ�ȭ
	_pdh_Value_Memory_UsePrivateBytes = 0;
	_sys_CPU_UseTotal = 0;
	_sys_CPU_UseUser = 0;
	_sys_CPU_UseKernel = 0;
	_sys_CPU_LastTime.QuadPart = 0;
	_sys_CPU_LastKernel.QuadPart = 0;
	_sys_CPU_LastUser.QuadPart = 0;
}
ProcessUsage::~ProcessUsage()
{
	// PDH ���� �ڵ� �ݱ�
	PdhCloseQuery(_pdhQuery);
}
void ProcessUsage::Update()
{
	// PDH ���ҽ� �� ����
	UpdatePDHValue();

	// System ���ҽ� �� ����
	UpdateSystemValue();
}
float ProcessUsage::GetUseCPUTotalTime()
{
	return (float)_sys_CPU_UseTotal;
}
float ProcessUsage::GetUseCPUUserTime()
{
	return (float)_sys_CPU_UseUser;
}
float ProcessUsage::GetUseCPUKernelTime()
{
	return (float)_sys_CPU_UseKernel;
}
float ProcessUsage::GetUseMemoryMBytes()
{
	return (float)(_pdh_Value_Memory_UsePrivateBytes / (1000 * 1000));
}
void ProcessUsage::InitPDHCounter()
{
	//---------------------------------------------------------------------------
	// �����ϰ����ϴ� ������ ����Ѵ�.
	// ���� ��� �� ī���� �ڵ��� 1���� ������ ���� ������ ����.
	//---------------------------------------------------------------------------
	WCHAR szQuery[1024];
	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Private Bytes", _processName);
	PdhAddCounter(_pdhQuery, szQuery, NULL, &_pdh_Counter_Memory_UsePrivateBytes);
}
void ProcessUsage::InitSystemInfo()
{
	//---------------------------------------------------------------------------
	// ���μ��� ������ Ȯ���Ѵ�.
	// ���μ��� (exe) ����� ���� cpu ������ �����⸦ �Ͽ� ���� ������ ����.
	//---------------------------------------------------------------------------
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	_numberOfProcessors = systemInfo.dwNumberOfProcessors;
}
void ProcessUsage::UpdatePDHValue()
{
	//---------------------------------------------------------------------------
	// ������ PDH ���ҽ� ������ �����Ѵ�.
	//---------------------------------------------------------------------------
	PDH_FMT_COUNTERVALUE pdhCounterValue;

	//---------------------------------------------------------------------------
	// PDH ���ҽ� ī���� ����
	//---------------------------------------------------------------------------
	PdhCollectQueryData(_pdhQuery);
	
	//---------------------------------------------------------------------------
	// Memory ��뷮 ���
	//---------------------------------------------------------------------------
	if (PdhGetFormattedCounterValue(_pdh_Counter_Memory_UsePrivateBytes, PDH_FMT_DOUBLE, NULL, &pdhCounterValue) == ERROR_SUCCESS)
		_pdh_Value_Memory_UsePrivateBytes = pdhCounterValue.doubleValue;
	else
		_pdh_Value_Memory_UsePrivateBytes = 0.0f;
}
void ProcessUsage::UpdateSystemValue()
{
	//---------------------------------------------------------------------------
	// ������ ���μ��� ������ �����Ѵ�.
	//---------------------------------------------------------------------------
	ULARGE_INTEGER none;
	ULARGE_INTEGER nowTime;
	ULARGE_INTEGER kernel;
	ULARGE_INTEGER user;
	ULONGLONG total;
	ULONGLONG timeDiff;
	ULONGLONG userDiff;
	ULONGLONG kernelDiff;

	//---------------------------------------------------------------------------
	// ������ 100 ���뼼���� ���� �ð��� ���Ѵ�. UTC �ð�.
	//
	// ���μ��� ���� �Ǵ��� ����
	//
	// a = ���ð����� �ý��� �ð��� ����. (�׳� ������ ������ �ð�)
	// b = ���μ����� CPU ��� �ð��� ����.
	//
	// a : 100 = b : ���� �������� ������ ����.
	//---------------------------------------------------------------------------
	// ���� �ð��� �������� 100 ���뼼���� �ð��� ���Ѵ�.
	//---------------------------------------------------------------------------
	GetSystemTimeAsFileTime((LPFILETIME)&nowTime);

	//---------------------------------------------------------------------------
	// �ش� ���μ����� ����� �ð��� ����.
	// �ι�°, ����°�� ����, ���� �ð����� �̻��.
	//---------------------------------------------------------------------------
	GetProcessTimes(_hProcess, (LPFILETIME)&none, (LPFILETIME)&none, (LPFILETIME)&kernel, (LPFILETIME)&user);

	//---------------------------------------------------------------------------
	// ������ ����� ���μ��� �ð����� ���� ���ؼ� ������ ���� �ð��� �������� Ȯ��.
	// �׸��� ���� ������ �ð����� ������ ������ ���´�.
	//---------------------------------------------------------------------------
	timeDiff = nowTime.QuadPart - _sys_CPU_LastTime.QuadPart;
	userDiff = user.QuadPart - _sys_CPU_LastUser.QuadPart;
	kernelDiff = kernel.QuadPart - _sys_CPU_LastKernel.QuadPart;
	total = kernelDiff + userDiff;

	_sys_CPU_UseTotal = total / (double)_numberOfProcessors / (double)timeDiff * 100.0f;
	_sys_CPU_UseUser = kernelDiff / (double)_numberOfProcessors / (double)timeDiff * 100.0f;
	_sys_CPU_UseKernel = userDiff / (double)_numberOfProcessors / (double)timeDiff * 100.0f;

	_sys_CPU_LastTime = nowTime;
	_sys_CPU_LastUser = user;
	_sys_CPU_LastKernel = kernel;
}
