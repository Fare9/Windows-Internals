#pragma once
#ifndef AUTOLOCK_H
#define AUTOLOCK_H

template<typename TLock>
struct AutoLock
{
public:
	AutoLock(TLock& lock) : _lock(lock)
	{
		lock.Lock();
	}

	~AutoLock()
	{
		_lock.Unlock();
	}
private:
	TLock& _lock;
};

#endif //!AUTOLOCK_H
