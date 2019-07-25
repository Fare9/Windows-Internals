#pragma once

#ifndef YOUSHALLNOTPASS_H
#define YOUSHALLNOTPASS_H

#include "FastMutex.h"


#define DRIVER_TAG 'NICE'
#define DRIVER_PREFIX "Gandalf Says: "


typedef struct _Globals
{
	LIST_ENTRY ItemsHead;
	int ItemCount;
	FastMutex Mutex;

	void Init()
	{
		// Initialize linked list head
		InitializeListHead(&ItemsHead);
		// Initialize Item counter
		ItemCount = 0;
		// Initialize Fastmutex
		Mutex.Init();
	}

} Globals, *PGlobals;


// template for linked list
// this shouldn't be exposed
// to user mode
template <typename T>
struct FullItem
{
	LIST_ENTRY Entry;
	T Data;
};


#endif // !YOUSHALLNOTPASS_H
