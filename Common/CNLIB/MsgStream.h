//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include <assert.h>
#include "HeapAlloc.h"
#include <string>
#include <vector>

#define MSG_OFFSET 2

class MsgStream
{
public:
	MsgStream(char* data, UINT capacity)
		:
		begin(data),
		end((char*)begin + capacity),
		data((char*)begin + MSG_OFFSET),
		capacity(capacity)
	{}
	MsgStream(MsgStream&& stream)
		:
		begin(stream.begin),
		end(stream.end),
		data(stream.data),
		capacity(stream.capacity)
	{
		ZeroMemory(&stream, sizeof(MsgStream));
	}

	~MsgStream()
	{
		dealloc((char*&)begin);
	}

	char GetType() const
	{
		return data[0];
	}
	char GetMsg() const
	{
		return data[1];
	}

	UINT GetSize() const
	{
		//+2 for MSG_OFFSET
		return end - begin;
	}
	UINT GetDataSize() const
	{
		//size without MSG_OFFSET
		return end - (begin + MSG_OFFSET);
	}
	UINT GetCapacity() const
	{
		return capacity;
	}

	template<typename T> T operator[](UINT index) const
	{
		assert(index <= capactiy - 2);
		return begin[index + 2];
	}

	void SetPos(UINT position)
	{
		assert(position <= capacity - 2);
		data = (char*)begin + position + MSG_OFFSET;
	}
	bool End() const
	{
		return begin == end;
	}

protected:
	const char* begin, *end;
	char* data;
	const UINT capacity;
};

class MsgStreamWriter : public MsgStream
{
public:
	//capacity not including MSG_OFFSET
	MsgStreamWriter(char type, char msg, UINT capacity)
		:
		MsgStream(alloc<char>(capacity + MSG_OFFSET), capacity)
	{
		data[0] = type;
		data[1] = msg;
	}
	MsgStreamWriter(char type, char msg, char* ptr, UINT nBytes)
		:
		MsgStream(ptr, nBytes)
	{
		data[0] = type;
		data[1] = msg;
	}
	MsgStreamWriter(MsgStreamWriter&& stream)
		:
		MsgStream(std::move(stream))
	{}

	operator const char*()
	{
		return begin;
	}

	template<typename T> void Write(const T& t)
	{
		Helper<T>(*this).Write(t);
	}
	template<typename T> void Write(T* t, UINT count)
	{
		Helper<T>(*this).Write(t, count);
	}
	template<typename T> void WriteEnd(T* t)
	{
		Helper<T>(*this).WriteEnd(t);
	}
	template<typename T> MsgStreamWriter& operator<<(const T& t)
	{
		return Helper<T>().operator<<(t);
	}

	template<typename T> UINT SizeType(const T& t) const
	{
		return Helper<T>(*this).Value(t);
	}
private:
	template<typename T> class Helper
	{
	public:
		Helper(MsgStreamWriter& stream)
			:
			stream(stream)
		{}

		void Write(const T& t)
		{
			*(T*)(stream.data) = t;
			stream.data += sizeof(T);
			assert(stream.begin <= stream.end);
		}
		void Write(T* t, UINT count)
		{
			const UINT nBytes = count * sizeof(T);
			memcpy(stream.data, t, nBytes);
			stream.data += nBytes;
			assert(stream.begin <= stream.end);
		}
		void WriteEnd(T* t)
		{
			const UINT nBytes = (stream.end + MSG_OFFSET) - stream.data;
			memcpy(stream.data, t, nBytes);
			stream.data += nBytes;
		}

		UINT SizeType(T* t, UINT count) const
		{
			UINT size = 0;
			for (UINT i = 0; i < count; i++)
				size += stream.SizeType(t[i]);

			return size;
		}
		UINT SizeType(const T& t) const
		{
			return sizeof(T);
		}

		MsgStreamWriter& operator<<(const T& t)
		{
			Write(t);
			return stream;
		}
	private:
		MsgStreamWriter& stream;
	};
};

class MsgStreamReader : public MsgStream
{
public:
	MsgStreamReader(char* data, UINT capacity)
		:
		MsgStream(data, capacity)
	{}
	MsgStreamReader(MsgStreamWriter&& stream)
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
		Helper<T>(*this).Read(t);
	}
	template<typename T> T* Read(UINT count)
	{
		return Helper<T>(*this).Read(count);
	}
	template<typename T> T* ReadEnd()
	{
		return Helper<T>(*this).ReadEnd();
	}
	template<typename T> MsgStreamReader& operator>>(T& dest)
	{
		return Helper<T>(*this).operator>>(dest);
	}
private:
	template<typename T> class Helper
	{
	public:
		Helper(MsgStreamReader& stream)
			:
			stream(stream)
		{}

		T Read()
		{
			T t = *(T*)(stream.data);
			stream.data += sizeof(T);
			assert(begin <= end);

			return t;
		}
		void Read(T& t)
		{
			t = *(T*)(stream.data);
			stream.data += sizeof(T);
			assert(stream.begin <= stream.end);
		}
		T* Read(UINT count)
		{
			const UINT nBytes = count * sizeof(T);
			T* t = (T*)(stream.data);
			stream.data += nBytes;
			assert(stream.begin <= stream.end);

			return t;
		}
		T* ReadEnd()
		{
			const UINT nBytes = (stream.end + MSG_OFFSET) - stream.data;
			T* t = (T*)(stream.data);
			stream.data += nBytes;

			return t;
		}

		MsgStreamReader& operator>>(T& dest)
		{
			Read<T>(dest);
			return stream;
		}
	private:
		MsgStreamReader& stream;
	};
};

//template<typename T> void MsgStreamWriter::Helper<std::vector<T>>::Write(const std::vector<T>& t);
//template<typename T> UINT MsgStreamWriter::Helper<std::vector<T>>::SizeType(const std::vector<T>& t) const;

template<> void MsgStreamWriter::Helper<std::string>::Write(const std::string& t);
template<> UINT MsgStreamWriter::Helper<std::string>::SizeType(const std::string& t) const;

template<> void MsgStreamWriter::Helper<std::wstring>::Write(const std::wstring& t);
template<> UINT MsgStreamWriter::Helper<std::wstring>::SizeType(const std::wstring& t) const;


template<> void MsgStreamReader::Helper<std::string>::Read(std::string& t);
template<> void MsgStreamReader::Helper<std::wstring>::Read(std::wstring& t);