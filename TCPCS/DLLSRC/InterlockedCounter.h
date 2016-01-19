#pragma once
#include "Typedefs.h"

class InterlockedCounter
{
public:
	InterlockedCounter()
		:
		opCount(0)
	{}

	inline InterlockedCounter& operator+=(UINT value)
	{
		InterlockedExchangeAdd(&opCount, value);
		return *this;
	}
	inline InterlockedCounter& operator-=(UINT value)
	{
		InterlockedExchangeSubtract(&opCount, value);
		return *this;
	}

	inline InterlockedCounter& operator++()
	{
		InterlockedIncrement(&opCount);
		return *this;
	}
	inline InterlockedCounter& operator--()
	{
		InterlockedDecrement(&opCount);
		return *this;
	}

	inline InterlockedCounter operator++(int)
	{
		InterlockedCounter temp = *this;
		InterlockedIncrement(&opCount);
		return temp;
	}
	inline InterlockedCounter operator--(int)
	{
		InterlockedCounter temp = *this;
		InterlockedDecrement(&opCount);
		return temp;
	}

	inline UINT GetOpCount() const
	{
		return opCount;
	}
	inline operator UINT() const
	{
		return opCount;
	}
private:
	UINT opCount;
};