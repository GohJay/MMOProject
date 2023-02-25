#ifndef __PROCESSER_USAGE__H_
#define __PROCESSER_USAGE__H_
#include <Pdh.h>

#define MAX_PDH_ETHERNET		8

namespace Jay
{
	class ProcesserUsage
	{
		/**
		* @file		ProcesserUsage.h
		* @brief	Processer Performance Usage Class
		* @details	프로세서 성능 모니터 클래스
		* @author	고재현
		* @date		2023-02-10
		* @version	1.0.0
		**/
	private:
		struct ETHERNET
		{
			bool used;
			WCHAR name[128];
			PDH_HCOUNTER pdh_Counter_Network_RecvBytes;
			PDH_HCOUNTER pdh_Counter_Network_SendBytes;
		};
	public:
		ProcesserUsage();
		~ProcesserUsage();
	public:
		void Update();
		float GetUseCPUTotalTime();
		float GetUseCPUUserTime();
		float GetUseCPUKernelTime();
		float GetUseNonpagedPoolMBytes();
		float GetFreeMemoryMBytes();
		float GetNetworkRecvMbit();
		float GetNetworkSendMbit();
	private:
		void AddEthernetCounter();
		void InitPDHCounter();
		void UpdatePDHValue();
		void UpdateSystemValue();
	private:
		PDH_HQUERY _pdhQuery;
		ETHERNET _ethernet[MAX_PDH_ETHERNET];
		PDH_HCOUNTER _pdh_Counter_NPPool_UseBytes;
		PDH_HCOUNTER _pdh_Counter_Memory_FreeMBytes;
		double _pdh_Value_Network_RecvBytes;
		double _pdh_Value_Network_SendBytes;
		double _pdh_Value_NPPool_UseBytes;
		double _pdh_Value_Memory_FreeMBytes;
		double _sys_CPU_UseTotal;
		double _sys_CPU_UseUser;
		double _sys_CPU_UseKernel;
		ULARGE_INTEGER _sys_CPU_LastIdle;
		ULARGE_INTEGER _sys_CPU_LastUser;
		ULARGE_INTEGER _sys_CPU_LastKernel;
	};
}

#endif
