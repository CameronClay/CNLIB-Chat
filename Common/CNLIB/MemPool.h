#pragma once
#include <stdlib.h>
#include <vector>
#include "HeapAlloc.h"

class MemPool
{
public:
	struct Element
	{
		Element()
			:
			prev(nullptr),
			next(nullptr)
		{}

		Element *prev, *next;

		static const size_t OFFSET = 2 * sizeof(Element*);
	};

	MemPool(size_t elementSizeMax, size_t count)
		:
		elementSizeMax(elementSizeMax),
		count(count),
		data(::alloc<char>((elementSizeMax + Element::OFFSET) * count)),
		begin((Element*)data),
		end((Element*)(data + (elementSizeMax + Element::OFFSET) * (count - 1))),
		used(nullptr),
		avail(begin)
	{
		memset(data, 0, (elementSizeMax + Element::OFFSET) * count);

		if (count > 1)
		{
			begin->prev = nullptr;
			begin->next = (Element*)(data + (elementSizeMax + Element::OFFSET));
			for (char *prev = (char*)begin, *ptr = (char*)(begin->next), *next = ptr + (elementSizeMax + Element::OFFSET);
				ptr != (char*)end;
				prev = ptr, ptr = next, next += (elementSizeMax + Element::OFFSET))//loop from 1 to count -1
			{
				Element& temp = *(Element*)(ptr);
				temp.prev = (Element*)prev;
				temp.next = (Element*)next;
			}
			end->prev = (Element*)(end - (elementSizeMax + Element::OFFSET));
			end->next = nullptr;
		}
	}
	MemPool(const MemPool&) = delete;
	MemPool(MemPool&& memPool)
		:
		elementSizeMax(memPool.elementSizeMax),
		count(memPool.count),
		data(memPool.data),
		begin(memPool.begin),
		end(memPool.end),
		used(memPool.used),
		avail(memPool.avail)
	{
		memset(&memPool, 0, sizeof(memPool));
	}

	MemPool& operator=(MemPool&& memPool)
	{
		if (this != &memPool)
		{
			const_cast<size_t&>(elementSizeMax) = memPool.elementSizeMax;
			const_cast<size_t&>(count) = memPool.count;
			data = memPool.data;
			begin = memPool.begin;
			end = memPool.end;
			used = memPool.used;
			avail = memPool.avail;

			memset(&memPool, 0, sizeof(memPool));
		}
		return *this;
	}

	~MemPool()
	{
		if (data)
		{
			::dealloc(data);
		}
	}

	template<typename T>
	T* alloc()
	{
		T* rtn = nullptr;
		if (avail && (sizeof(T) <= elementSizeMax))
		{
			Element* element = avail;
			//Remove element from available list
			if (avail->next)
			{
				avail = avail->next;
				avail->prev = nullptr;
			}
			else
			{
				avail = nullptr;
			}

			//Add element to used list
			element->next = used;
			element->prev = nullptr;
			used = element;

			rtn = (T*)((char*)element + Element::OFFSET);
		}
		else
		{
			rtn = ::alloc<T>();
		}

		return rtn;
	}

	template<typename T>
	void dealloc(T*& t)
	{
		if (t)
		{
			Element* e = (Element*)((char*)t - Element::OFFSET);

			//If allocated by mempool and size is less than maximum element size
			if ((e >= begin && e <= end) && (sizeof(T) <= elementSizeMax))
			{
				//Remove element from used list
				if (e->prev)
					e->prev->next = e->next;
				else if (e->next)
					used = e->next;
				else
					used = nullptr;

				//Add element to available list
				if (avail)
				{
					e->next = avail;
					e->prev = nullptr;
					avail = avail->prev = e;
				}
				else
				{
					e->next = nullptr;
					e->prev = nullptr;
					avail = e;
				}
			}
			else
			{
				::dealloc(t);
			}

			t = nullptr;
		}
	}

	template<typename T, typename... Args>
	inline T* construct(Args&&... vals)
	{
		return pmconstruct<T>(alloc<T>(), std::forward<Args>(vals)...);
	}

	template<typename T>
	inline void destruct(T*& p)
	{
		if (p)
		{
			pmdestruct(p);
			dealloc(p);
		}
	}

	inline size_t ElementSizeMax() const
	{
		return elementSizeMax;
	}
	inline size_t Count() const
	{
		return count;
	}

	template<typename T>
	inline bool InPool(T* p) const
	{
		Element* e = (Element*)((char*)p - Element::OFFSET);
		return e >= begin && e <= end;
	}
	inline bool IsFull() const
	{
		return !avail;
	}
	inline bool IsEmpty() const
	{
		return !used;
	}
private:
	const size_t elementSizeMax, count;
	char* data;
	Element *begin, *end;
	Element *used, *avail;
};

class MemPoolSync : MemPool
{
public:
	MemPoolSync(size_t elementSizeMax, size_t count)
		:
		MemPool(elementSizeMax, count)
	{
		InitializeCriticalSection(&sect);
	}
	MemPoolSync(const MemPool&) = delete;
	MemPoolSync(MemPoolSync&& memPool)
		:
		MemPool(std::forward<MemPool>(*this)),
		sect(memPool.sect)
	{
		memset(&memPool, 0, sizeof(memPool));
	}

	MemPoolSync& operator=(MemPoolSync&& memPool)
	{
		if (this != &memPool)
		{
			__super::operator=(std::forward<MemPool>(*this));
			sect = memPool.sect;

			memset(&memPool, 0, sizeof(memPool));
		}
		return *this;
	}

	~MemPoolSync()
	{
		DeleteCriticalSection(&sect);
	}

	template<typename T>
	T* alloc(bool sync = true)
	{
		if (sync)
			EnterCriticalSection(&sect);

		T* rtn = __super::alloc<T>();

		if (sync)
			LeaveCriticalSection(&sect);

		return rtn;
	}

	template<typename T>
	void dealloc(T*& t, bool sync = true)
	{
		if (sync)
			EnterCriticalSection(&sect);
		
		__super::dealloc(t);
			
		if (sync)
			LeaveCriticalSection(&sect);
	}

	template<typename T, typename... Args>
	inline T* construct(bool sync, Args&&... vals)
	{
		return pmconstruct<T>(alloc<T>(sync), std::forward<Args>(vals)...);
	}

	template<typename T>
	inline void destruct(T*& p, bool sync = true)
	{
		if (p)
		{
			pmdestruct(p);
			dealloc(p, sync);
		}
	}

private:
	CRITICAL_SECTION sect;
};