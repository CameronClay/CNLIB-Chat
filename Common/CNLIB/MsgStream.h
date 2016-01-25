//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "HeapAlloc.h"
#include "Messages.h"

class MsgStream
{
public:
	MsgStream(char* data, UINT capacity)
		:
		begin(data),
		end(data + capacity + MSG_OFFSET),
		data(data + MSG_OFFSET)
	{}
	MsgStream(MsgStream&& stream)
		:
		begin(stream.begin),
		end(stream.end),
		data(stream.data)
	{
		ZeroMemory(&stream, sizeof(MsgStream));
	}

	short GetType() const
	{
		return *(short*)begin;
	}
	short GetMsg() const
	{
		return *((short*)begin + 1);
	}

	UINT GetSize() const
	{
		return end - begin;
	}
	UINT GetDataSize() const
	{
		//size without MSG_OFFSET
		return end - (begin + MSG_OFFSET);
	}

	template<typename T> std::enable_if_t<std::is_arithmetic<T>::value, T> operator[](UINT index) const
	{
		assert(index <= capacity - MSG_OFFSET);
		return begin[index + MSG_OFFSET];
	}

	bool End() const
	{
		return data == end;
	}

protected:
	const char* begin, *end;
	char* data;
};

typedef class MsgStreamWriter : public MsgStream
{
public:
	//capacity not including MSG_OFFSET
	MsgStreamWriter(short type, short msg, UINT capacity)
		:
		MsgStream(alloc<char>(capacity + MSG_OFFSET), capacity)
	{
		((short*)data)[-2] = type;
		((short*)data)[-1] = msg;
	}
	MsgStreamWriter(MsgStreamWriter&& stream)
		:
		MsgStream(std::move(stream))
	{}

	~MsgStreamWriter()
	{
		dealloc( (char*&)begin );
	}

	operator const char*()
	{
		return begin;
	}

	template<typename T> void Write(const T& t)
	{
		Helper<T>(*this).Write(t);
	}
	template<typename T> MsgStreamWriter& operator<<(const T& t)
	{
		Write(t);
		return *this;
	}

	template<typename T>
	std::enable_if_t<std::is_arithmetic<T>::value> Write(T* t, UINT count)
	{
		Helper<T>(*this).Write(t, count);
	}
	template<typename T>
	std::enable_if_t<std::is_arithmetic<T>::value> WriteEnd(T* t)
	{
		Helper<T>(*this).WriteEnd(t);
	}

	//no constexpr in vs13 :(
	template<typename... T>
	static inline UINT SizeType()
	{
		return TypeSize<T...>::value;
	}

	template<typename... T>
	static UINT SizeType(const T&... t)
	{
		UINT size = 0;
		for(auto& a : { Helper<T>::SizeType(t)... })
			size += a;
		return size;
	}

private:
	template<typename T, typename... V>
	struct TypeSize : std::integral_constant<UINT, TypeSize<T>::value + TypeSize<V...>::value>{};
	template<typename T>
	struct TypeSize<T> : std::integral_constant<UINT, sizeof(T)>
	{ static_assert(std::is_arithmetic<T>::value, "cannot call SizeType<T...>() with a non-arithmetic type"); };

	template<typename T> class HelpBase
	{
	public:
		HelpBase(MsgStreamWriter& stream)
			:
			stream(stream)
		{}
		HelpBase operator=(const HelpBase) = delete;
	protected:
		MsgStreamWriter& stream;
	};
	template<typename T> class Helper : public HelpBase<T>
	{
	public:
		Helper(MsgStreamWriter& stream)
			:
			HelpBase(stream)
		{}

		void Write(const T& t)
		{
			*(T*)(stream.data) = t;
			stream.data += sizeof(T);
			assert(stream.data <= stream.end);
		}
		void Write(T* t, UINT count)
		{
			const UINT nBytes = count * sizeof(T);
			memcpy(stream.data, t, nBytes);
			stream.data += nBytes;
			assert(stream.data <= stream.end);
		}
		void WriteEnd(T* t)
		{
			const UINT nBytes = stream.end - stream.data;
			memcpy(stream.data, t, nBytes);
			stream.data += nBytes;
			assert(stream.data <= stream.end);
		}
		static UINT SizeType(const T&)
		{
			return sizeof(T);
		}
	};
} StreamWriter;

typedef class MsgStreamReader : public MsgStream
{
public:
	MsgStreamReader(char* data, UINT capacity)
		:
		MsgStream(data, capacity)
	{}
	MsgStreamReader(MsgStreamReader&& stream)
		:
		MsgStream(std::move(stream))
	{}

	operator const char*()
	{
		return begin;
	}

	template<typename T> T Read()
	{
		return Helper<T>(*this).Read();
	}
	template<typename T> void Read(T& t)
	{
		t = Helper<T>(*this).Read();
	}
	template<typename T> MsgStreamReader& operator>>(T& dest)
	{
		Read(dest);
		return *this;
	}

	template<typename T> 
	std::enable_if_t<std::is_arithmetic<T>::value, T>* Read(UINT count)
	{
		return Helper<T>(*this).Read(count);
	}
	template<typename T> 
	std::enable_if_t<std::is_arithmetic<T>::value, T>* ReadEnd()
	{
		return Helper<T>(*this).ReadEnd();
	}
private:
	template<typename T> class HelpBase
	{
	public:
		HelpBase(MsgStreamReader& stream)
			:
			stream(stream)
		{}
		HelpBase operator=(HelpBase) = delete;
	protected:
		MsgStreamReader& stream;
	};
	template<typename T> class Helper : public HelpBase<T>
	{
	public:
		Helper(MsgStreamReader& stream)
			:
			HelpBase(stream)
		{}

		T Read()
		{
			T t = *(T*)stream.data;
			stream.data += sizeof(T);
			assert(stream.data <= stream.end);

			return t;
		}
		T* Read(UINT count)
		{
			const UINT nBytes = count * sizeof(T);
			T* t = (T*)(stream.data);
			stream.data += nBytes;
			assert(stream.data <= stream.end);

			return t;
		}
		T* ReadEnd()
		{
			const UINT nBytes = stream.end - stream.data;
			T* t = (T*)(stream.data);
			stream.data += nBytes;
			assert(stream.data <= stream.end);

			return t;
		}
	};
} StreamReader;

#include "StreamExt.h"
