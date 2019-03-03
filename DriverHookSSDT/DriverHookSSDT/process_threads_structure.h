#pragma once

#ifndef PROCESS_THREADS_STRUCTURE_H
#define PROCESS_THREADS_STRUCTURE_H

#include "common.h"

/*
*	Structures used for the next ntdll.dll function:
*		ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength)
*
*	Both _SYSTEM_THREADS and _SYSTEM_PROCESS
*	These structures will be present when the parameter SystemInformationClass has the value 5 (SystemProcessInformation)
*
*	In the _SYSTEM_PROCESSOR_TIMES case, we will have it when SystemInformationClass has the value 8 (SystemProcessorPerformanceInformation)
*	(more information in chapter 11 from The Rootkit Arsenal)
*/

#define SYSTEMPROCESSINFORMATION 5
#define SYSTEMPROCESSORPERFORMANCEINFORMATION 8

typedef struct _SYSTEM_THREADS
{
	LARGE_INTEGER			KernelTime;
	LARGE_INTEGER			UserTime;
	LARGE_INTEGER			CreateTime;
	ULONG					WaitTime;
	PVOID					StartAddress;
	CLIENT_ID				ClientIs;
	KPRIORITY				Priority;
	KPRIORITY				BasePriority;
	ULONG					ContextSwitchCount;
	ULONG					ThreadState;
	KWAIT_REASON			WaitReason;
} SYSTEM_THREADS, *PSYSTEM_THREADS;


typedef struct _SYSTEM_PROCESSES
{
	ULONG					NextEntryDelta;
	ULONG					ThreadCount;
	ULONG					Reserved[6];
	LARGE_INTEGER			CreateTime;
	LARGE_INTEGER			UserTime;
	LARGE_INTEGER			KernelTime;
	UNICODE_STRING			ProcessName;
	KPRIORITY				BasePriority;
	ULONG					ProcessId;
	ULONG					InheritedFromProcessId;
	ULONG					HandleCount;
	ULONG					Reserved2[2];
	VM_COUNTERS				VmCounters;
	IO_COUNTERS				IoCounters; // only for windows 2000
	struct _SYSTEM_THREADS	Threads[1];
} SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;


typedef struct _SYSTEM_PROCESSOR_TIMES
{
	LARGE_INTEGER					IdleTime;
	LARGE_INTEGER					KernelTime;
	LARGE_INTEGER					UserTime;
	LARGE_INTEGER					DpcTime;
	LARGE_INTEGER					InterruptTime;
	ULONG							InterruptCount;
} SYSTEM_PROCESSOR_TIMES, *PSYSTEM_PROCESSOR_TIMES;

#endif // !PROCESS_THREADS_STRUCTURE_H
