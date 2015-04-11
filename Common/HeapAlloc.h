#pragma once

#include <stdlib.h>
#include <Windows.h>
#include <memory>

//---------------------------------------------DOCUMENTATION---------------------------------------------
//alloc and dealloc are together, for blank memory : new type var or new type[count] : alloc<type>() or alloc<type>(count) : uqp/m_sp
//construct and destruct are together, for a single constructed element : new type var(value) : construct(type()) or construct<type>(value) : uqpc/m_csp
//constructa and destructa are together. for a constructed array : new type[count] var{v1,v2,v3} : constructa<type>(value, value) or constructa<type>(type(), type()) : uqpca/m_casp

template<typename T> inline T* alloc()
{
	return (T*)HeapAlloc(GetProcessHeap(), NULL, sizeof( T ));
}
template<typename T> inline T* alloc( size_t count )
{
	return (T*)(count != 0 ? HeapAlloc(GetProcessHeap(), NULL, sizeof(T) * count) : nullptr);
}
template<typename T> inline void dealloc( T& p )
{
	if( p )
	{
		HeapFree(GetProcessHeap(), NULL, p);
		p = nullptr;
	}
}
template<typename T> inline T* construct( T&& t )
{
	return new(alloc<T>()) T( std::forward<T>( t ) );
}
template<typename T> inline void destruct( T*& p )
{
	if( p )
	{
		p->~T();
		HeapFree( GetProcessHeap(), NULL, p );
		p = nullptr;
	}
}
template<typename T, typename... A> T* constructa( A&&... vals )
{
	const size_t count = sizeof...(vals);
	if( count != 0 )
	{
		T* p = nullptr;
		if( std::is_integral<T>::value ) p = (T*)HeapAlloc( GetProcessHeap(), NULL, sizeof( T ) * count );
		else
		{
			p = (T*)HeapAlloc( GetProcessHeap(), NULL, (sizeof( T ) * count) + sizeof( size_t ) );
			*((size_t*)p) = count;
		}
		return new(p)T[]{std::forward<T>( vals )...};
	}
	return nullptr;
}
template<typename T> void destructa( T*& p )
{
	if( p )
	{
		size_t* s = (size_t*)p;
		if( !std::is_integral<T>::value )
		{
			const size_t hSize = HeapSize( GetProcessHeap(), NULL, --s ) - sizeof( size_t );
			const size_t size = hSize / *s;
			for( char* i = (char*)p, *e = i + hSize; i != e; i += size ) ((T*)i)->~T();
		}
		HeapFree( GetProcessHeap(), NULL, s );
		p = nullptr;
	}
}

template<class T> class allocator
{
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	allocator() {}
	allocator( reference p ) {}
	~allocator() {}
	static inline const_pointer address( const_reference p )
	{
		return &p;
	}
	static inline pointer address( reference p )
	{
		return &p;
	}
	template <class U> struct rebind
	{
		typedef allocator<U> other;
	};
	static inline size_t max_size()
	{
		return MAXSIZE_T;
	}
	static inline pointer allocate( size_type n )
	{
		return alloc<T>( n );
	}
	static inline void deallocate( pointer p, size_type n = 0 )
	{
		dealloc( p );
	}
	static inline void construct( pointer p, const_reference v )
	{
		new(p)T( v );
	}
	static inline void destroy( pointer p )
	{
		p->~T();
	}
};
template<class T1, class T2> bool operator==(const allocator<T1>&, const allocator<T2>&)
{
	return true;
}
template<class T1, class T2> bool operator!=(const allocator<T1>&, const allocator<T2>&)
{
	return false;
}

template<typename T> struct deleter
{
	deleter() throw() {}
	template<typename T2> deleter( const deleter<T2>&, typename std::enable_if<std::is_convertible<T2*, T*>::value>::type** = 0 ) throw() {}
	void operator()( T* t ) throw()
	{
		static_assert(sizeof( T ), "can't delete an incomplete type");
		dealloc( t );
	}
};
template<typename T> struct deleterc
{
	deleterc() throw() {}
	template<typename T2> deleterc( const deleterc<T2>&, typename std::enable_if<std::is_convertible<T2*, T*>::value>::type** = 0 ) throw() {}
	void operator()( T* t ) throw()
	{
		static_assert(sizeof( T ), "can't delete an incomplete type");
		destruct( t );
	}
};
template<typename T> struct deleterca
{
	deleterca() throw() {}
	template<typename T2> deleterca( const deleterc<T2>&, typename std::enable_if<std::is_convertible<T2*, T*>::value>::type** = 0 ) throw() {}
	void operator()( T* t ) throw()
	{
		static_assert(sizeof( T ), "can't delete an incomplete type");
		destructa( t );
	}
};

template<typename T> using uqp = std::unique_ptr < T, deleter<T> >;
template<typename T> using uqpc = std::unique_ptr < T, deleterc<T> >;
template<typename T> using uqpca = std::unique_ptr < T, deleterca<T> >;

template<typename T> std::shared_ptr<T> m_sp( T* p )
{
	return{ p, deleter<T>() };
}
template<typename T> std::shared_ptr<T> m_csp( T* p )
{
	return{ p, deleterc<T>() };
}
template<typename T> std::shared_ptr<T> m_casp( T* p )
{
	return{ p, deleterca<T>() };
}

//template< typename T > class pool
//{
//public:
//	pool(size_t maxElements, size_t initialElements)
//		:
//		maxE(maxElements),
//		curE(initialElements),
//		tSize(sizeof(T))
//	{
//		data = (T*)HeapAlloc(GetProcessHeap(), NUll, initialElements * tSize)
//	}
//	~pool()
//	{}
//	void ReSize(size_t nElements)
//	{
//		curSize = sizeof(T)* nElements;
//		data = (TCHAR*)HeapReAlloc(GetProcessHeap(), NULL, data, curSize);
//	}
//	void Add()
//	{
//		ReSize(1);
//	}
//	T* GetPointer()
//	{
//		if (Growable(tSize))
//		{
//			T* p = (T*)data;
//			++data;
//			return p;
//		}
//		return nullptr;
//	}
//	T* GetPointer(size_t nElements)
//	{
//		if (Growable(nElements))
//		{
//			T* p = (T*)data;
//			data += nElements;
//			return p;
//		}
//		return nullptr;
//	}
//	inline bool Growable(size_t nElements)
//	{
//		return curE + nElements < maxE;
//	}
//	inline bool Empty()
//	{
//		return curE == 0;
//	}
//	void Clear()
//	{
//		if (data)
//			HeapFree(GetProcessHeap(), NULL, data)
//	}
//private:
//	T* data;
//	const size_t maxE, tSize;
//	size_t curE;
//};
