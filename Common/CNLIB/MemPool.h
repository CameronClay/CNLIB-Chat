#pragma once
#include <stdlib.h>
#include <vector>
#include "HeapAlloc.h"

#pragma once
#include <stdlib.h>
#include <vector>
#include "HeapAlloc.h"

//If using custom allocator, you must use type of char for its template type
template<typename Allocator = std::allocator<char>>
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

		Element *prev, *next;

		static const size_t OFFSET = 2 * sizeof(Element*);
	};

	//capacity must be >= 1
	explicit MemPool(size_t elementSizeMax, size_t capacity, const Allocator& alloc = Allocator())
		:
		allocator(alloc),
		elementSizeMax(elementSizeMax),
		capacity(capacity),
		data(allocator.allocate((elementSizeMax + Element::OFFSET) * capacity)),
		begin((Element*)data),
		end((Element*)(data + (elementSizeMax + Element::OFFSET) * (capacity - 1))),
		used(nullptr),
		avail(begin)
	{
		memset(data, 0, (elementSizeMax + Element::OFFSET) * capacity);

		if (capacity > 1)
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
		allocator(std::move(memPool.allocator)),
		elementSizeMax(memPool.elementSizeMax),
		capacity(memPool.capacity),
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
				allocator = std::move(memPool.allocator);
				const_cast<size_t&>(elementSizeMax) = memPool.elementSizeMax;
				const_cast<size_t&>(capacity) = memPool.capacity;
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
				allocator.deallocate(data, NULL);
		}

		void* alloc(size_t elementSize)
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
				Element* element = (Element*)((char*)t - Element::OFFSET);

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
			Element* e = (Element*)((char*)p - Element::OFFSET);
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

		inline bool IsEmpty() const
		{
			return !used;
		}
		inline bool IsNotEmpty() const
		{
			return used;
		}

		typedef Allocator allocator_type;
protected:
	inline void* PoolAlloc()
	{
		Element* element = PopAvail();
		PushUsed(element);
		return (void*)((char*)element + Element::OFFSET);
	}
	inline void PoolDealloc(Element* element)
	{
		EraseUsed(element);
		PushAvail(element);
	}

	bool ElementInPool(Element* element) const
	{
		return element >= begin && element <= end;
	}

	Allocator allocator;

	const size_t elementSizeMax, capacity;
	char* data;
	Element *begin, *end;
	Element *used, *avail;
private:
	//Remove element from front available list
	Element* PopAvail()
	{
		Element* element = avail;
		if (avail->next)
		{
			avail = avail->next;
			avail->prev = nullptr;
		}
		else
		{
			avail = nullptr;
		}
		return element;
	}
	//Remove element from anywhere in used list
	void EraseUsed(Element* element)
	{
		if (element == used)
			used = element->next;
		if (element->prev)
			element->prev->next = element->next;
		if (element->next)
			element->next->prev = element->prev;
		if (!(element->prev || element->next))
			used = nullptr;
	}
	//Add element to front of available list
	void PushAvail(Element* element)
	{
		if (avail)
		{
			element->next = avail;
			element->prev = nullptr;
			avail = avail->prev = element;
		}
		else
		{
			element->next = nullptr;
			element->prev = nullptr;
			avail = element;
		}
	}
	//Add element to front of used list
	void PushUsed(Element* element)
	{
		if (used)
			used->prev = element;
		element->next = used;
		element->prev = nullptr;
		used = element;
	}
};

//If using custom allocator, you must use type of char for its template type
template<typename Allocator = std::allocator<char>>
//Fixed-Sized Synced Memory Pool
class MemPoolSync : public MemPool<Allocator>
{
public:
	//capacity must be >= 1
	explicit MemPoolSync(size_t elementSizeMax, size_t capacity, const Allocator& allocator = Allocator())
		:
		MemPool(elementSizeMax, capacity, allocator)
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
			Element* element = (Element*)((char*)t - Element::OFFSET);

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