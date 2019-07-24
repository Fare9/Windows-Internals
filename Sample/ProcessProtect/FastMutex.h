#pragma once
#ifndef FASTMUTEX_H
#define FASTMUTEX_H

#include "pch.h"

// Base class for FastMutex, this is not secure.
class FastMutex
{
public:

	void Init();

	void Lock();
	void Unlock();

private:
	FAST_MUTEX _mutex;
};


#endif // !FASTMUTEX_H
