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

	// PDH 쿼리 핸들 열기
	PdhOpenQuery(NULL, NULL, &_pdhQuery);

	// PDH 리소스 카운터 생성
	InitPDHCounter();

	// System 리소스 정보 얻기
	InitSystemInfo();

	// 리소스 값 초기화
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
	// PDH 쿼리 핸들 닫기
	PdhCloseQuery(_pdhQuery);
}
void ProcessUsage::Update()
{
	// PDH 리소스 값 갱신
	UpdatePDHValue();

	// System 리소스 값 갱신
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
	// 수집하고자하는 쿼리를 등록한다.
	// 수집 대상 당 카운터 핸들이 1개씩 나오며 개수 제한은 없다.
	//---------------------------------------------------------------------------
	WCHAR szQuery[1024];
	StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Private Bytes", _processName);
	PdhAddCounter(_pdhQuery, szQuery, NULL, &_pdh_Counter_Memory_UsePrivateBytes);
}
void ProcessUsage::InitSystemInfo()
{
	//---------------------------------------------------------------------------
	// 프로세서 개수를 확인한다.
	// 프로세스 (exe) 실행률 계산시 cpu 개수로 나누기를 하여 실제 사용률을 구함.
	//---------------------------------------------------------------------------
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	_numberOfProcessors = systemInfo.dwNumberOfProcessors;
}
void ProcessUsage::UpdatePDHValue()
{
	//---------------------------------------------------------------------------
	// 지정된 PDH 리소스 사용률을 갱신한다.
	//---------------------------------------------------------------------------
	PDH_FMT_COUNTERVALUE pdhCounterValue;

	//---------------------------------------------------------------------------
	// PDH 리소스 카운터 갱신
	//---------------------------------------------------------------------------
	PdhCollectQueryData(_pdhQuery);
	
	//---------------------------------------------------------------------------
	// Memory 사용량 얻기
	//---------------------------------------------------------------------------
	if (PdhGetFormattedCounterValue(_pdh_Counter_Memory_UsePrivateBytes, PDH_FMT_DOUBLE, NULL, &pdhCounterValue) == ERROR_SUCCESS)
		_pdh_Value_Memory_UsePrivateBytes = pdhCounterValue.doubleValue;
	else
		_pdh_Value_Memory_UsePrivateBytes = 0.0f;
}
void ProcessUsage::UpdateSystemValue()
{
	//---------------------------------------------------------------------------
	// 지정된 프로세스 사용률을 갱신한다.
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
	// 현재의 100 나노세컨드 단위 시간을 구한다. UTC 시간.
	//
	// 프로세스 사용률 판단의 공식
	//
	// a = 샘플간격의 시스템 시간을 구함. (그냥 실제로 지나간 시간)
	// b = 프로세스의 CPU 사용 시간을 구함.
	//
	// a : 100 = b : 사용률 공식으로 사용률을 구함.
	//---------------------------------------------------------------------------
	// 얼마의 시간이 지났는지 100 나노세컨드 시간을 구한다.
	//---------------------------------------------------------------------------
	GetSystemTimeAsFileTime((LPFILETIME)&nowTime);

	//---------------------------------------------------------------------------
	// 해당 프로세스가 사용한 시간을 구함.
	// 두번째, 세번째는 실행, 종료 시간으로 미사용.
	//---------------------------------------------------------------------------
	GetProcessTimes(_hProcess, (LPFILETIME)&none, (LPFILETIME)&none, (LPFILETIME)&kernel, (LPFILETIME)&user);

	//---------------------------------------------------------------------------
	// 이전에 저장된 프로세스 시간과의 차를 구해서 실제로 얼마의 시간이 지났는지 확인.
	// 그리고 실제 지나온 시간으로 나누면 사용률이 나온다.
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
