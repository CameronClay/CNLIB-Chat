#include "StdAfx.h"
#include "CNLIB/CritLock.h"

CritLock::CritLock()
	:
	valid(true)
{
	InitializeCriticalSection(&lock);
}

CritLock::CritLock(CritLock&& lock)
	:
	lock(lock.lock),
	valid(lock.valid)
{
	lock.valid = false;
}
CritLock& CritLock::operator=(CritLock&& lock)
{
	if (this != &lock)
	{
		this->lock = lock.lock;
		valid = lock.valid;
		lock.valid = false;
	}
	return *this;
}
CritLock::~CritLock()
{
	if (valid)
	{
		DeleteCriticalSection(&lock);
		valid = false;
	}
}

void CritLock::Lock()
{
	EnterCriticalSection(&lock);
}
void CritLock::Unlock()
{
	LeaveCriticalSection(&lock);
}

bool CritLock::Valid() const
{
	return valid;
}

