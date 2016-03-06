#pragma once
#include <Windows.h>
#include <memory>

template<template<typename> class Allocator = std::allocator>
//Fixed-Sized Memory Pool
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

		Element *next;

		static const size_t OFFSET = sizeof(Element*);
	};

	//capacity must be >= 1
	//note alignment only gaurentees alignment based on the address given from alloc.allocate(), in order to ensure proper allignment, 
	//allocator should also align to alignment passed here
	explicit MemPool(size_t elementSize, size_t capacity, size_t alignment = 4, const Allocator<char>& alloc = Allocator<char>())
		:
		allocator(alloc),
		elementSizeMax((((((elementSize + Element::OFFSET + alignment - 1) / alignment)) * alignment)) - Element::OFFSET), //round up to nearest multiple of alignment
		capacity(capacity),
		data(allocator.allocate((elementSizeMax + Element::OFFSET) * capacity)),
		begin((Element*)(data + elementSizeMax)),
		end((Element*)((char*)begin + (elementSizeMax + Element::OFFSET) * (capacity - 1))),
		avail(begin)
	{
		memset(data, 0, (elementSizeMax + Element::OFFSET) * capacity);

		if (capacity > 1)
		{
			begin->next = (Element*)((char*)begin + (elementSizeMax + Element::OFFSET));
			for (char *ptr = (char*)(begin->next), *next = ptr + (elementSizeMax + Element::OFFSET);
				ptr != (char*)end;
				ptr = next, next += (elementSizeMax + Element::OFFSET))//loop from 1 to count -1
			{
				Element& temp = *(Element*)(ptr);
				temp.next = (Element*)next;
			}
			end->next = nullptr;
		}
	}
	MemPool(const MemPool&) = delete;
	MemPool(MemPool&& memPool)
		:
		allocator(std::move(memPool.allocator)),
		elementSizeMax(memPool.elementSizeMax),
		capacity(memPool.capacity),
		data(memPool.data),
		begin(memPool.begin),
		end(memPool.end),
		avail(memPool.avail)
	{
		memset(&memPool, 0, sizeof(memPool));
	}

	MemPool& operator=(MemPool&& memPool)
	{
		if (this != &memPool)
		{
			allocator = std::move(memPool.allocator);
			const_cast<size_t&>(elementSizeMax) = memPool.elementSizeMax;
			const_cast<size_t&>(capacity) = memPool.capacity;
			data = memPool.data;
			begin = memPool.begin;
			end = memPool.end;
			avail = memPool.avail;

			memset(&memPool, 0, sizeof(memPool));
		}
		return *this;
	}

	~MemPool()
	{
		if (data)
			allocator.deallocate(data, NULL);
	}

	inline void* alloc(size_t elementSize)
	{
		if (IsNotFull() && FitsInPool(elementSize))
			return PoolAlloc();
		else
			return (void*)allocator.allocate(max(elementSize, elementSizeMax));
	}

	template<typename T>
	inline T* alloc()
	{
		return (T*)alloc(sizeof(T));
	}

	template<typename T>
	void dealloc(T*& t)
	{
		if (t)
		{
			Element* element = (Element*)((char*)t + elementSizeMax);

			if (ElementInPool(element))
				PoolDealloc(element);
			else
				allocator.deallocate((char*)t, NULL);

			t = nullptr;
		}
	}

	template<typename T, typename... Args>
	inline T* construct(Args&&... vals)
	{
		return new(alloc<T>()) T(std::forward<Args>(vals)...);
	}

	template<typename T>
	inline void destruct(T*& p)
	{
		if (p)
		{
			p->~T();
			dealloc(p);
		}
	}

	inline size_t ElementSizeMax() const
	{
		return elementSizeMax;
	}
	inline size_t Capacity() const
	{
		return capacity;
	}

	template<typename T>
	inline bool InPool(T* p) const
	{
		Element* e = (Element*)((char*)p + elementSizeMax);
		return (e >= begin) && (e <= end);
	}
	inline bool FitsInPool(size_t elementSize) const
	{
		return elementSize <= elementSizeMax;
	}

	inline bool IsFull() const
	{
		return !avail;
	}
	inline bool IsNotFull() const
	{
		return avail;
	}

	typedef Allocator<char> allocator_type;
public:
	inline void* PoolAlloc()
	{
		Element* element = PopAvail();
		return (void*)((char*)element - elementSizeMax);
	}
	inline void PoolDealloc(Element* element)
	{
		PushAvail(element);
	}

	inline bool ElementInPool(Element* element) const
	{
		return element >= begin && element <= end;
	}

	Allocator<char> allocator;

	const size_t elementSizeMax, capacity;
	char* data;
	Element *begin, *end;
	Element *used, *avail;
private:
	//Remove element from front available list
	inline Element* PopAvail()
	{
		Element *element = avail, *next = avail->next;

		avail = next ? next : nullptr;

		return element;
	}
	//Add element to front of available list
	inline void PushAvail(Element* element)
	{
		element->next = avail ? avail : nullptr;
		avail = element;
	}
};

template<template<typename> class Allocator = std::allocator>
//Fixed-Sized Synced Memory Pool
class MemPoolSync : public MemPool<Allocator>
{
public:
	//capacity must be >= 1
	//note alignment only gaurentees alignment based on the address given from alloc.allocate(), in order to ensure proper allignment, 
	//allocator should also align to alignment passed here
	explicit MemPoolSync(size_t elementSize, size_t capacity, size_t alignment = 4, const Allocator<char>& allocator = Allocator<char>())
		:
		MemPool(elementSize, capacity, alignment, allocator)
	{
		InitializeCriticalSection(&sect);
	}
	MemPoolSync(const MemPoolSync&) = delete;
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

	void* alloc(size_t elementSize)
	{
		if (IsNotFull() && FitsInPool(elementSize))
		{
			EnterCriticalSection(&sect);
			void* ptr = PoolAlloc();
			LeaveCriticalSection(&sect);
			return ptr;
		}
		else
		{
			return (void*)allocator.allocate(max(elementSize, elementSizeMax));
		}
	}

	template<typename T>
	inline T* alloc()
	{
		return (T*)alloc(sizeof(T));
	}

	template<typename T>
	void dealloc(T*& t)
	{
		if (t)
		{
			Element* element = (Element*)((char*)t + elementSizeMax);

			if (ElementInPool(element))
			{
				EnterCriticalSection(&sect);
				PoolDealloc(element);
				LeaveCriticalSection(&sect);
			}
			else
			{
				allocator.deallocate((char*)t, NULL);
			}

			t = nullptr;
		}
	}

	template<typename T, typename... Args>
	inline T* construct(Args&&... vals)
	{
		return new(alloc<T>()) T(std::forward<Args>(vals)...);
	}

	template<typename T>
	inline void destruct(T*& p)
	{
		if (p)
		{
			p->~T();
			dealloc(p);
		}
	}

private:
	CRITICAL_SECTION sect;
};