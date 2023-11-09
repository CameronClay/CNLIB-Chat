#pragma once
#include "HeapAlloc.h"
#include "Messages.h"
#include "CNLIB/BuffSendInfo.h"
#include <cstddef>

class MsgStream
{
public:
	MsgStream(char* data, UINT capacity)
		:
		begin(data),
		end(data + capacity + MSG_OFFSET),
		data(data + MSG_OFFSET),
		capacity(capacity)
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

	std::size_t GetSize() const
	{
		return end - begin;
	}
	std::size_t GetDataSize() const
	{
		//size without MSG_OFFSET
		return end - (begin + MSG_OFFSET);
	}

	template<typename T> std::enable_if_t<std::is_arithmetic<T>::value, T> operator[](std::size_t index) const
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
	std::size_t capacity;
};

class MsgStreamWriter : public MsgStream
{
public:
	//capacity not including MSG_OFFSET
	MsgStreamWriter(const BuffSendInfo& buffSendInfo, UINT capacity, short type, short msg)
		:
		MsgStream(buffSendInfo.buffer, capacity),
		buffSendInfo(buffSendInfo)
	{
		((short*)begin)[0] = type;
		((short*)begin)[1] = msg;
	}
	MsgStreamWriter(MsgStreamWriter&& stream)
		:
		MsgStream(std::move(stream)),
		buffSendInfo(stream.buffSendInfo)
	{}

	operator const char*() const
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

	char* GetData() const
	{
		return (char*)begin;
	}

	std::size_t GetSize() const
	{
		return data - begin;
	}
	std::size_t GetDataSize() const
	{
		//size without MSG_OFFSET
		return data - (begin + MSG_OFFSET);
	}

    template<typename... Ts>
	static inline constexpr std::size_t SizeType()
	{
        return (sizeof(Ts) + ...);
        //return TypeSize<Ts...>::value;
	}

    template<typename... Ts>
    static constexpr std::size_t SizeType(const Ts&... ts)
	{
		std::size_t size = 0;
        for (auto& a : { Helper<Ts>::SizeType(ts)... }) {
			size += a;
		}
		return size;
	}

	BuffSendInfo GetBuffSendInfo() const
	{
		return buffSendInfo;
	}

private:
	BuffSendInfo buffSendInfo;

	template<typename T, typename... V>
	struct TypeSize : std::integral_constant<std::size_t, TypeSize<T>::value + TypeSize<V...>::value>{};
	template<typename T>
	struct TypeSize<T> : std::integral_constant<std::size_t, sizeof(T)>
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
			HelpBase<T>(stream)
		{}

		void Write(const T& t)
		{
			*(T*)(this->stream.data) = t;
			this->stream.data += sizeof(T);
			assert(this->stream.data <= this->stream.end);
		}
		void Write(T* t, UINT count)
		{
			const UINT nBytes = count * sizeof(T);
			memcpy(this->stream.data, t, nBytes);
			this->stream.data += nBytes;
			assert(this->stream.data <= this->stream.end);
		}
		void WriteEnd(T* t)
		{
			const UINT nBytes = this->stream.end - this->stream.data;
			memcpy(this->stream.data, t, nBytes);
			this->stream.data += nBytes;
			assert(this->stream.data <= this->stream.end);
		}
		static constexpr std::size_t SizeType(const T&)
		{
			return sizeof(T);
		}
	};
};
using StreamWriter = MsgStreamWriter;

class MsgStreamReader : public MsgStream
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

	char* GetData()
	{
		return (char*)(begin + MSG_OFFSET);
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
	std::enable_if_t<std::is_arithmetic<T>::value, T>* Read(std::size_t count)
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
			HelpBase<T>(stream)
		{}

		T Read()
		{
			T t = *(T*)this->stream.data;
			this->stream.data += sizeof(T);
			assert(this->stream.data <= this->stream.end);

			return t;
		}
		T* Read(UINT count)
		{
			const UINT nBytes = count * sizeof(T);
			T* t = (T*)(this->stream.data);
			this->stream.data += nBytes;
			assert(this->stream.data <= this->stream.end);

			return t;
		}
		T* ReadEnd()
		{
			const UINT nBytes = this->stream.end - this->stream.data;
			T* t = (T*)(this->stream.data);
			this->stream.data += nBytes;
			assert(this->stream.data <= this->stream.end);

			return t;
		}
	};
};
using StreamReader = MsgStreamReader;

#include "StreamExt.h"
