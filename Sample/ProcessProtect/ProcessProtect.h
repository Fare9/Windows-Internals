#pragma once

#ifndef PROCESSPROTECT_H
#define PROCESSPROTECT_H


#define DRIVER_PREFIX "ProcessProtect: "

#define PROCESS_TERMINATE 1

#include "FastMutex.h"

const int MaxPids = 256;

typedef struct _Globals
{
	int			PidsCount;		// Currently protected process count
	ULONG		Pids[MaxPids];	// protected PIDs
	FastMutex	Lock;
	PVOID		RegHandle;		// object registration cookie

	void Init()
	{
		Lock.Init();
	}
} Globals, *PGlobals;


#endif // !PROCESSPROTECT_H
