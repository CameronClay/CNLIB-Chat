//Copyright (c) <2015> <Cameron Clay>

#pragma once

template<template<typename> class Allocator = std::allocator>
//Growable single-thread safe memory pool
class MemPool
{
public:
	struct Element
	{
		Element()
			:
			next(nullptr)
		{}

		Element *next;

		static const size_t OFFSET = sizeof(Element*);
	};

	struct Node
	{
		Node(Allocator<char>& allocator, Element*& avail, size_t elementSizeMax, size_t capacity)
			:
			bufferSize((elementSizeMax + Element::OFFSET) * capacity),
			data(allocator.allocate(bufferSize)),
			begin((Element*)(data + elementSizeMax)),
			end((Element*)((char*)begin + bufferSize - (elementSizeMax + Element::OFFSET)))
		{
			Link(avail, elementSizeMax, capacity);
		}

		Node(const Node& node)
			:
			data(node.data),
			begin(node.begin),
			end(node.end)
		{}

		Node& operator=(const Node& node)
		{
			if (this != &node)
			{
				data = node.data;
				begin = node.begin;
				end = node.end;
			}
			return *this;
		}

		void Link(Element*& avail, size_t elementSizeMax, size_t capacity)
		{
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
			}
			end->next = avail;
			avail = begin;
		}

		void FreeNode(Allocator<char>& allocator)
		{
			if (data)
				allocator.deallocate(data, bufferSize);
		}

		template<typename T>
		inline bool InNode(T* p, size_t elementSizeMax) const
		{
			return ElementInNode((char*)p + elementSizeMax);
		}

		inline bool ElementInNode(Element* e) const
		{
			return (e >= begin) && (e <= end);
		}

		size_t bufferSize;
		char* data;
		Element *begin, *end;
	};

	//capacity must be >= 1
	//note alignment only gaurentees alignment based on the address given from alloc.allocate(), in order to ensure proper allignment, 
	//allocator should also align to alignment passed here
	explicit MemPool(size_t elementSize, size_t initialCapacity, size_t alignment = 4, const Allocator<char>& alloc = Allocator<char>())
		:
		avail(nullptr),
		nodes(),
		allocator(alloc),
		elementSizeMax((((((elementSize + Element::OFFSET + alignment - 1) / alignment)) * alignment)) - Element::OFFSET), //round up to nearest multiple of alignment
		capacity(initialCapacity)
	{
		nodes.reserve(5);
		nodes.emplace_back(allocator, avail, elementSizeMax, initialCapacity);
	}
	MemPool(const MemPool&) = delete;
	MemPool(MemPool&& memPool)
		:
		avail(memPool.avail),
		nodes(std::move(memPool.nodes)),
		allocator(std::move(memPool.allocator)),
		elementSizeMax(memPool.elementSizeMax),
		capacity(memPool.capacity)
	{
		memset(&memPool, 0, sizeof(MemPool));
	}

	MemPool& operator=(MemPool&& memPool)
	{
		if (this != &memPool)
		{
			avail = memPool.avail;
			nodes = std::move(memPool.nodes);
			allocator = std::move(memPool.allocator);
			const_cast<size_t&>(elementSizeMax) = memPool.elementSizeMax;
			capacity = memPool.capacity;

			memset(&memPool, 0, sizeof(MemPool));
		}
		return *this;
	}

	~MemPool()
	{
		if (elementSizeMax)
			for (auto& it : nodes)
				it.FreeNode(allocator);
	}

	void* alloc(size_t elementSize)
	{
		if (FitsInPool(elementSize))
			return PoolAlloc();

		return nullptr;
	}

	template<typename T>
	inline T* alloc()
	{
		return (T*)alloc(sizeof(T));
	}

	template<typename T, typename... Args>
	inline T* construct(Args&&... vals)
	{
		return new(alloc<T>()) T(std::forward<Args>(vals)...);
	}

	template<typename T>
	void dealloc(T*& p)
	{
		if (p)
		{
			Element* element = (Element*)((char*)p + elementSizeMax);
			PoolDealloc(element);
			p = nullptr;
		}
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

	template<typename T>
	bool InPool(T* p) const
	{
		Element* element = (Element*)((char*)p + elementSizeMax);
		for (int i = 0, size = nodes.size(); i < size; i++)
		{
			if (nodes[i].ElementInNode(element))
				return true;
		}
		return false;
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

	void Reserve(size_t capacity)
	{
		Grow(capacity);
	}

	size_t ElementSizeMax() const
	{
		return elementSizeMax;
	}
	size_t Capacity() const
	{
		return capacity;
	}
	size_t NodeCount() const
	{
		return nodes.size();
	}

	typedef Allocator<char> allocator_type;
protected:
	inline void* PoolAlloc()
	{
		if (IsFull())
			Grow(capacity);

		return (void*)((char*)PopAvail() - elementSizeMax);
	}
	inline void PoolDealloc(Element* element)
	{
		PushAvail(element);
	}

	Element* avail;
	std::vector<Node> nodes;
	Allocator<char> allocator;
	const size_t elementSizeMax;
	size_t capacity;
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

	//Add an additonal node, capacity is the amount to increase by not total capacity
	inline void Grow(size_t capacity)
	{
		nodes.emplace_back(allocator, avail, elementSizeMax, capacity);
		this->capacity += capacity;
	}
};

template<template<typename> class Allocator = std::allocator>
//Growable multi-thread safe memory pool
class MemPoolSync : public MemPool<Allocator>
{
public:
	//capacity must be >= 1
	//note alignment only gaurentees alignment based on the address given from alloc.allocate(), in order to ensure proper allignment, 
	//allocator should also align to alignment passed here
	explicit MemPoolSync(size_t elementSize, size_t initialCapacity, size_t alignment = 4, const Allocator<char>& allocator = Allocator<char>())
		:
		MemPool<Allocator>(elementSize, initialCapacity, alignment, allocator)
	{
		InitializeCriticalSection(&sect);
	}
	MemPoolSync(const MemPoolSync&) = delete;
	MemPoolSync(MemPoolSync&& memPool)
		:
		MemPool<Allocator>(std::move(memPool)),
		sect(memPool.sect)
	{
		memset(&memPool, 0, sizeof(MemPoolSync));
	}

	MemPoolSync& operator=(MemPoolSync&& memPool)
	{
		if (this != &memPool)
		{
			MemPool<Allocator>::operator=(std::move(memPool));
			sect = memPool.sect;

			memset(&memPool, 0, sizeof(MemPoolSync));
		}
		return *this;
	}

	~MemPoolSync()
	{
		if (this->elementSizeMax)
			DeleteCriticalSection(&sect);
	}

	void* alloc(size_t elementSize)
	{
		void* ptr = nullptr;
		if (this->FitsInPool(elementSize))
		{
			EnterCriticalSection(&sect);
			ptr = this->PoolAlloc();
			LeaveCriticalSection(&sect);
		}
		return ptr;
	}

	template<typename T>
	inline T* alloc()
	{
		return (T*)alloc(sizeof(T));
	}

	template<typename T, typename... Args>
	inline T* construct(Args&&... vals)
	{
		return new(alloc<T>()) T(std::forward<Args>(vals)...);
	}

	template<typename T>
	void dealloc(T*& p)
	{
		if (p)
		{
			typename MemPool<Allocator>::Element* element = (typename MemPool<Allocator>::Element*)((char*)p + this->elementSizeMax);

			EnterCriticalSection(&sect);
			this->PoolDealloc(element);
			LeaveCriticalSection(&sect);

			p = nullptr;
		}
	}

	template<typename T>
	inline void destruct(T*& p)
	{
		if (p)
		{
			p->~T();
			this->dealloc(p);
		}
	}

	void lock()
	{
		EnterCriticalSection(&sect);
	}

	void unlock()
	{
		LeaveCriticalSection(&sect);
	}
private:
	CRITICAL_SECTION sect;
};