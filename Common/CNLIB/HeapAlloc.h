//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Typedefs.h"

//---------------------------------------------DOCUMENTATION---------------------------------------------
//alloc and dealloc are together, for blank memory : new type var or new type[count] : alloc<type>() or alloc<type>(count) : uqp/m_sp
//construct and destruct are together, for a single constructed element : new type var(value) : construct<type>(...) or construct<type>(value) : uqpc/m_csp
//constructa and destructa are together, for a constructed array : new type[count] var{v1,v2,v3} : constructa<type>(value, value) or constructa<type>(type(), type()) : uqpca/m_casp
//pmconstruct and pmdestruct are together, for constructing at a specified memory region : new (ptr) T() : pmconstruct<type>(ptr, ...) or pmconstruct <type>(ptr, value) : pmuqp/m_pmsp

class CAMSNETLIB Alloc
{
public:
	Alloc() = default;
	Alloc(size_t initialSize, size_t maxSize) : heap(HeapCreate(0, initialSize, maxSize))
	{
		ULONG ptr = 2;
		HeapSetInformation(heap, HeapCompatibilityInformation, &ptr, sizeof(ULONG));
	}
	Alloc(HANDLE heap) : heap(heap){}
	Alloc(const Alloc&) = delete;
	Alloc(Alloc&& al) :heap(al.heap)
	{
		memset(&al, 0, sizeof(al));
	}
	~Alloc()
	{
		if (heap && heap != GetProcessHeap())
			HeapDestroy(heap);
	}

	template<typename T>
	inline T* alloc(size_t count = 1) const
	{
		return (T*)(count != 0 ? HeapAlloc(heap, HEAP_GENERATE_EXCEPTIONS, sizeof(T) * count) : nullptr);
	}

	template<typename T>
	inline void dealloc(T*& p) const
	{
		if (p)
		{
			HeapFree(heap, NULL, p);
			p = nullptr;
		}
	}

	template<typename T, typename... Args>
	inline T* construct(Args&&... vals) const
	{
		return pmconstruct<T>(alloc<T>(), std::forward<Args>(vals)...);
	}

	template<typename T>
	inline void destruct(T*& p) const
	{
		pmdestruct(p);
		dealloc(p);
	}

	template<typename T, typename... Args>
	inline T* constructa(Args&&... vals) const
	{
		T *p = alloc<T>(sizeof...(vals));
		if (!std::is_trivially_destructible<T>())
		{
			size_t size = HeapSize(heap, 0, p);
			p = (T*)HeapReAlloc(heap, 0, p, sizeof(size_t) + size);
		}
		return pmconstructa<T>(p, std::forward<Args>(vals)...);
	}

	template<typename T>
	inline void destructa(T*& p) const
	{
		pmdestructa(p);
		dealloc(p);
	}


private:
	const HANDLE heap = GetProcessHeap();
};

extern const CAMSNETLIB Alloc ProcHeap;

template<typename T>
inline T* alloc(size_t count = 1)
{
	return ProcHeap.alloc<T>(count);
}

template<typename T>
inline void dealloc(T*& p)
{
	ProcHeap.dealloc(p);
}

template<typename T, typename... Args>
inline T* construct(Args&&... vals)
{
	return ProcHeap.construct<T>(std::forward<Args>(vals)...);
}

template<typename T>
inline void destruct(T*& p)
{
	ProcHeap.destruct(p);
}

template<typename T, typename... Args>
inline T* constructa(Args&&... vals)
{
	return ProcHeap.constructa<T>(std::forward<Args>(vals)...);
}

template<typename T>
inline void destructa(T*& p)
{
	ProcHeap.destructa(p);
}

template<typename T, typename P, typename... Args> inline T* pmconstruct(P* ptr, Args&&... vals)
{
	return new(ptr) T(std::forward<Args>(vals)...);
}
template<typename T> inline void pmdestruct(T* p)
{
	if (p)
		p->~T();
}

template<typename T, typename... Args> inline T* pmconstructa(T* ptr, Args&&... vals)
{
	return new(ptr)T[sizeof...(vals)]{std::forward<Args>(vals)...};
}
template<typename T> inline void pmdestructa(T*& p)
{
	if (p && !std::is_trivially_destructible<T>())
	{
		const size_t count = *((size_t*)p - 1);
		const size_t size = sizeof(T) * count + sizeof(size_t);
		for (T *e = p + count; p < e; p++)
			pmdestruct(p);
		p = (T*)((char*)p - size);
	}
}
template<class T> class HeapAllocator
{
public:
	typedef HeapAllocator<T> other;

	typedef T value_type;

	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef void *void_pointer;
	typedef const void *const_void_pointer;

	typedef value_type& reference;
	typedef const value_type& const_reference;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef std::true_type propagate_on_container_move_assignment;
	typedef std::true_type is_always_equal;

	HeapAllocator<T> select_on_container_copy_construction() const
	{
		return (*this);
	}

	template<class _Other> struct rebind
	{
		typedef HeapAllocator<_Other> other;
	};

	pointer address(reference _Val) const noexcept
	{
		return (_STD addressof(_Val));
	}

	const_pointer address(const_reference _Val) const noexcept
	{
		return (_STD addressof(_Val));
	}

	HeapAllocator() {}

	HeapAllocator(const HeapAllocator<T>&) {}

	template<class _Other> HeapAllocator(const HeapAllocator<_Other>&) {}

	template<class _Other> HeapAllocator<T>& operator=(const HeapAllocator<_Other>&)
	{
		return (*this);
	}

	size_t max_size() const
	{
		return ((size_t)(-1) / sizeof(T));
	}

	static inline pointer allocate(size_type n)
	{
		return alloc<T>(n);
	}
	static inline pointer allocate(size_type n, const void *)
	{
		return allocate(n);
	}
	static inline void deallocate(pointer p, size_type n = 0)
	{
		dealloc(p);
	}

	static inline void construct(pointer p, const_reference v)
	{
		pmconstruct<T>(p, v);
	}
	template<typename P, typename... Args> static inline void construct(P* ptr, Args&&... vals)
	{
		pmconstruct<P>(ptr, std::forward<Args>(vals)...);
	}

	template<typename P> static inline void destroy(P* ptr)
	{
		pmdestruct(ptr);
	}
};

template<class T1, class T2> bool operator==(const HeapAllocator<T1>&, const HeapAllocator<T2>&)
{
	return true;
}
template<class T1, class T2> bool operator!=(const HeapAllocator<T1>&, const HeapAllocator<T2>&)
{
	return false;
}

template<class T> class PageAllignAllocator
{
public:
	typedef PageAllignAllocator<T> other;

	typedef T value_type;

	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef void *void_pointer;
	typedef const void *const_void_pointer;

	typedef value_type& reference;
	typedef const value_type& const_reference;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef std::true_type propagate_on_container_move_assignment;
	typedef std::true_type is_always_equal;

	PageAllignAllocator<T> select_on_container_copy_construction() const
	{
		return (*this);
	}

	template<class _Other> struct rebind
	{
		typedef PageAllignAllocator<_Other> other;
	};

	pointer address(reference _Val) const noexcept
	{
		return (_STD addressof(_Val));
	}

	const_pointer address(const_reference _Val) const noexcept
	{
		return (_STD addressof(_Val));
	}

	PageAllignAllocator() {}

	PageAllignAllocator(const PageAllignAllocator<T>&) {}

	template<class _Other> PageAllignAllocator(const PageAllignAllocator<_Other>&) {}

	template<class _Other> PageAllignAllocator<T>& operator=(const PageAllignAllocator<_Other>&)
	{
		return (*this);
	}

	size_t max_size() const
	{
		return ((size_t)(-1) / sizeof(T));
	}

	static inline pointer allocate(size_type n)
	{
		return (pointer)VirtualAlloc(NULL, n * sizeof(T), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	}
	static inline pointer allocate(size_type n, const void *)
	{
		return allocate(n);
	}
	static inline void deallocate(pointer p, size_type n = 0)
	{
		VirtualFree(p, 0, MEM_RELEASE);
	}

	static inline void construct(pointer p, const_reference v)
	{
		pmconstruct<T>(p, v);
	}
	template<typename P, typename... Args> static inline void construct(P* ptr, Args&&... vals)
	{
		pmconstruct<P>(ptr, std::forward<Args>(vals)...);
	}

	template<typename P> static inline void destroy(P* ptr)
	{
		pmdestruct(ptr);
	}
};

template<class T1, class T2> bool operator==(const PageAllignAllocator<T1>&, const PageAllignAllocator<T2>&)
{
	return true;
}
template<class T1, class T2> bool operator!=(const PageAllignAllocator<T1>&, const PageAllignAllocator<T2>&)
{
	return false;
}

template<typename T> struct deleter
{
	deleter() throw() {}
	template<typename T2> deleter(const deleter<T2>&, typename std::enable_if<std::is_convertible<T2*, T*>::value>::type** = 0) throw() {}
	void operator()(T* t) throw()
	{
		static_assert(sizeof(T), "can't delete an incomplete type");
		dealloc(t);
	}
};
template<typename T> struct deleterc
{
	deleterc() throw() {}
	template<typename T2> deleterc(const deleterc<T2>&, typename std::enable_if<std::is_convertible<T2*, T*>::value>::type** = 0) throw() {}
	void operator()(T* t) throw()
	{
		static_assert(sizeof(T), "can't delete an incomplete type");
		destruct(t);
	}
};
template<typename T> struct pmdeleter
{
	pmdeleter() throw() {}
	template<typename T2> pmdeleter(const pmdeleter<T2>&, typename std::enable_if<std::is_convertible<T2*, T*>::value>::type** = 0) throw() {}
	void operator()(T* t) throw()
	{
		static_assert(sizeof(T), "can't delete an incomplete type");
		pmdestruct(t);
	}
};
template<typename T> struct deleterca
{
	deleterca() throw() {}
	template<typename T2> deleterca(const deleterca<T2>&, typename std::enable_if<std::is_convertible<T2*, T*>::value>::type** = 0) throw() {}
	void operator()(T* t) throw()
	{
		static_assert(sizeof(T), "can't delete an incomplete type");
		destructa(t);
	}
};

template<typename T> using uqp = std::unique_ptr < T, deleter<T> >;
template<typename T> using uqpc = std::unique_ptr < T, deleterc<T> >;
template<typename T> using uqpca = std::unique_ptr < T, deleterca<T> >;
template<typename T> using pmuqp = std::unique_ptr < T, pmdeleter<T> >;

template<typename T> std::shared_ptr<T> m_sp(T* p)
{
	return{ p, deleter<T>() };
}
template<typename T> std::shared_ptr<T> m_csp(T* p)
{
	return{ p, deleterc<T>() };
}
template<typename T> std::shared_ptr<T> m_casp(T* p)
{
	return{ p, deleterca<T>() };
}
template<typename T> std::shared_ptr<T> m_pmsp(T* p)
{
	return{ p, pmdeleter<T>() };
}
