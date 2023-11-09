#pragma once
#include "Typedefs.h"

class CAMSNETLIB CritLock
{
public:
	CritLock();
	CritLock(CritLock&& lock);
	CritLock& operator=(CritLock&& lock);
	~CritLock();

	void Lock();
	void Unlock();
	bool Valid() const;
private:
	CRITICAL_SECTION lock;
	int valid;
};