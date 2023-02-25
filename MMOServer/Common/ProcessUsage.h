#ifndef __PROCESS_USAGE__H_
#define __PROCESS_USAGE__H_
#include <Pdh.h>

namespace Jay
{
	class ProcessUsage
	{
		/**
		* @file		ProcessUsage.h
		* @brief	Process Performance Usage Class
		* @details	프로세스 성능 모니터 클래스
		* @author	고재현
		* @date		2023-02-10
		* @version	1.0.0
		**/
	public:
		ProcessUsage(HANDLE hProcess = INVALID_HANDLE_VALUE);
		~ProcessUsage();
	public:
		void Update();
		float GetUseCPUTotalTime();
		float GetUseCPUUserTime();
		float GetUseCPUKernelTime();
		float GetUseMemoryMBytes();
	private:
		void InitPDHCounter();
		void InitSystemInfo();
		void UpdatePDHValue();
		void UpdateSystemValue();
	private:
		PDH_HQUERY _pdhQuery;
		PDH_HCOUNTER _pdh_Counter_Memory_UsePrivateBytes;
		double _pdh_Value_Memory_UsePrivateBytes;
		double _sys_CPU_UseTotal;
		double _sys_CPU_UseUser;
		double _sys_CPU_UseKernel;
		ULARGE_INTEGER _sys_CPU_LastTime;
		ULARGE_INTEGER _sys_CPU_LastUser;
		ULARGE_INTEGER _sys_CPU_LastKernel;
		int _numberOfProcessors;
		HANDLE _hProcess;
		WCHAR _processName[128];
	};
}

#endif
