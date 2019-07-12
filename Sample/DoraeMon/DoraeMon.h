#pragma once

#include "FastMutex.h"


#ifndef DORAEMON_H
#define DORAEMON_H

#define DRIVER_TAG 'nome'
#define DRIVER_PREFIX "DoraeMon: "
#define DEVICE_NAME L"\\Device\\DoraeMon"
#define SYMBOLIC_NAME L"\\??\\DoraeMon"


typedef struct _Globals
{
	LIST_ENTRY ItemsHead;
	int ItemCount;
	FastMutex Mutex;
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


#endif // !DORAEMON_H
