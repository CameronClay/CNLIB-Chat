//Copyright (c) <2015> <Cameron Clay>

#pragma once

#include <stdlib.h>
#include <Windows.h>
#include <memory>

//---------------------------------------------DOCUMENTATION---------------------------------------------
//alloc and dealloc are together, for blank memory : new type var or new type[count] : alloc<type>() or alloc<type>(count) : uqp/m_sp
//construct and destruct are together, for a single constructed element : new type var(value) : construct<type>(...) or construct<type>(value) : uqpc/m_csp
//constructa and destructa are together, for a constructed array : new type[count] var{v1,v2,v3} : constructa<type>(value, value) or constructa<type>(type(), type()) : uqpca/m_casp
//pmconstruct and pmdestruct are together, for constructing at a specified memory region : new (ptr) T() : pmconstruct<type>(ptr, ...) or pmconstruct <type>(ptr, value) : pmuqp/m_pmsp

template<typename T> inline T* alloc()
{
	HANDLE heap = GetProcessHeap();
	T* t = (T*)HeapAlloc(heap, HEAP_GENERATE_EXCEPTIONS, sizeof(T));
	if ((int)t == HEAP_GENERATE_EXCEPTIONS || (int)t == HEAP_NO_SERIALIZE || (int)t == HEAP_ZERO_MEMORY)
	{
		int a = 0;
	}
	return t;
}
template<typename T> inline T* alloc(size_t count)
{
	HANDLE heap = GetProcessHeap();
	T* t = (T*)(count != 0 ? HeapAlloc(heap, HEAP_GENERATE_EXCEPTIONS, sizeof(T) * count) : nullptr);
	if ((int)t == HEAP_GENERATE_EXCEPTIONS || (int)t == HEAP_NO_SERIALIZE || (int)t == HEAP_ZERO_MEMORY)
	{
		int a = 0;
	}
	return t;
}
template<typename T> inline void dealloc(T& p)
{
	if (p)
	{
		HeapFree(GetProcessHeap(), NULL, p);
		p = nullptr;
	}
}

template<typename T, typename... Args> inline T* construct(Args&&... vals)
{
	return new(alloc<T>()) T(std::forward<Args>(vals)...);
}
template<typename T> inline T* construct(T&& obj)
{
	return new(alloc<T>()) T(std::forward<T>(obj));
}
template<typename T> inline void destruct(T*& p)
{
	if (p)
	{
		p->~T();
		HeapFree(GetProcessHeap(), NULL, p);
		p = nullptr;
	}
}

template<typename T, typename P, typename... Args> inline void pmconstruct(P* ptr, Args&&... vals)
{
	new(ptr) T(std::forward<Args>(vals)...);
}
template<typename T, typename P> inline void pmconstruct(P* ptr, T&& obj)
{
	new(ptr) T(std::forward<T>(obj));
}
template<typename T> inline void pmdestruct(T* p)
{
	if (p)
		p->~T();
}

template<typename T, typename... Args> T* constructa(Args&&... vals)
{
	const size_t count = sizeof...(vals);
	if (count != 0)
	{
		T* p = nullptr;
		if (std::is_integral<T>::value) p = (T*)HeapAlloc(GetProcessHeap(), NULL, sizeof(T) * count);
		else
		{
			p = (T*)HeapAlloc(GetProcessHeap(), NULL, (sizeof(T) * count) + sizeof(size_t));
			*((size_t*)p) = count;
		}
		return new(p)T[]{ std::forward<T>(vals)... };
	}
	return nullptr;
}
template<typename T> void destructa(T*& p)
{
	if (p)
	{
		size_t* s = (size_t*)p;
		if (!std::is_integral<T>::value)
		{
			const size_t hSize = HeapSize(GetProcessHeap(), NULL, --s) - sizeof(size_t);
			const size_t size = hSize / *s;
			for (char* i = (char*)p, *e = i + hSize; i != e; i += size) ((T*)i)->~T();
		}
		HeapFree(GetProcessHeap(), NULL, s);
		p = nullptr;
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

	pointer address(reference _Val) const _NOEXCEPT
	{	
		return (_STD addressof(_Val));
	}

	const_pointer address(const_reference _Val) const _NOEXCEPT
	{	
		return (_STD addressof(_Val));
	}

	HeapAllocator() _THROW0(){}

	HeapAllocator(const HeapAllocator<T>&) _THROW0(){}

	template<class _Other> HeapAllocator(const HeapAllocator<_Other>&) _THROW0(){}

	template<class _Other> HeapAllocator<T>& operator=(const HeapAllocator<_Other>&)
	{	
		return (*this);
	}

	size_t max_size() const _THROW0()
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
