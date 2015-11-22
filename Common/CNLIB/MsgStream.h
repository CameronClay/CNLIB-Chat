//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include <assert.h>
#include <string>
#include "HeapAlloc.h"

class MsgStream
{
public:
	//nBytes not including MSG_OFFSET
	MsgStream(char* data, DWORD nBytes)
		:
		pos(2),
		data(data),
		nBytes(nBytes)
	{}
	MsgStream(MsgStream&& stream)
		:
		pos(stream.pos),
		data(stream.data),
		nBytes(stream.nBytes)
	{
		ZeroMemory(&stream, sizeof(MsgStream));
	}

	char GetType() const
	{
		return data[0];
	}
	char GetMsg() const
	{
		return data[1];
	}

	DWORD GetSize() const
	{
		//+2 for MSG_OFFSET
		return nBytes + 2;
	}
	DWORD GetDataSize() const
	{
		//size without MSG_OFFSET
		return nBytes;
	}
	DWORD GetPos() const
	{
		return pos;
	}

	template<typename T> T& operator[](DWORD position) const
	{
		assert(position <= nBytes + 2);
		return data[position + 2];
	}

	void SetPos(DWORD position)
	{
		assert(pos <= nBytes + 2);
		pos = position;
	}
	bool End() const
	{
		assert(pos <= nBytes + 2);
		return pos == nBytes + 2;
	}

	DWORD operator+=(DWORD amount)
	{
		assert(pos + amount <= nBytes + 2);
		pos += amount;
		return pos;
	}
	DWORD operator-=(DWORD amount)
	{
		assert(pos - amount >= 2);
		pos -= amount;
		return pos;
	}
public:
	DWORD pos;
	char* data;
	const DWORD nBytes;
};

class MsgStreamWriter : public MsgStream
{
public:
	//nBytes not including MSG_OFFSET
	MsgStreamWriter(char type, char msg, DWORD nBytes)
		:
		MsgStream(alloc<char>(nBytes + 2), nBytes)
	{
		data[0] = type;
		data[1] = msg;
	}
	MsgStreamWriter(char type, char msg, char* ptr, DWORD nBytes)
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

	~MsgStreamWriter()
	{
		dealloc(data);
	}

	operator char*()
	{
		return data;
	}

	template<typename T> struct Helper
	{
		void Write(const T& t)
		{
			*(T*)(data + pos) = t;
			pos += sizeof(T);
			assert(pos <= nBytes + 2);
		}
		void Write(T* t, DWORD count)
		{
			const DWORD nBytes = count * sizeof(T);
			memcpy(data + pos, t, nBytes);
			pos += nBytes;
			assert(pos <= this->nBytes + 2);
		}
		void WriteEnd(T* t)
		{
			const DWORD nBytes = (this->nBytes + 2) - pos;
			memcpy(data + pos, t, nBytes);
			pos += nBytes;
			assert(pos <= this->nBytes + 2);
		}

		MsgStreamWriter& operator<<(const T& t)
		{
			Write(t);
			return *this;
		}
	};

	template<typename T> void Write(const T& t)
	{
		Helper<T>().Write(t);
	}
	template<typename T> void Write(T* t, DWORD count)
	{
		Helper<T>().Write(t, count);
	}
	template<typename T> void WriteEnd(T* t)
	{
		Helper<T>().WriteEnd(t);
	}
	template<typename T> MsgStreamWriter& operator<<(const T& t)
	{
		return Helper<T>().operator<<(t);
	}
};

template<> void MsgStreamWriter::Helper<std::string>::Write(const std::string& t);
template<> void MsgStreamWriter::Helper<std::wstring>::Write(const std::wstring& t);

class MsgStreamReader : public MsgStream
{
public:
	//nBytes not including MSG_OFFSET
	MsgStreamReader(char* data, DWORD nBytes)
		:
		MsgStream(data, nBytes)
	{}
	MsgStreamReader(MsgStreamWriter&& stream)
		:
		MsgStream(std::move(stream))
	{}

	operator char*()
	{
		return data;
	}

	template<typename T> struct Helper
	{
		T& Read()
		{
			T& t = *(T*)(data + pos);
			pos += sizeof(T);
			assert(pos <= nBytes + 2);

			return t;
		}
		T* Read(DWORD count)
		{
			const DWORD nBytes = count * sizeof(T);
			T* t = (T*)(data + pos);
			pos += nBytes;
			assert(pos <= this->nBytes + 2);

			return t;
		}
		T* ReadEnd()
		{
			const DWORD nBytes = (this->nBytes + 2) - pos;
			T* t = (T*)(data + pos);
			pos += nBytes;
			assert(pos <= this->nBytes + 2);

			return t;
		}

		MsgStreamReader& operator>>(T& dest)
		{
			dest = Read<T>();
			return *this;
		}
	};

	template<typename T> T& Read()
	{
		return Helper<T>().Read();
	}
	template<typename T> T* Read(DWORD count)
	{
		return Helper<T>().Read(count);
	}
	template<typename T> T* ReadEnd()
	{
		return Helper<T>().ReadEnd();
	}
	template<typename T> MsgStreamReader& operator>>(T& dest)
	{
		return Helper<T>().operator>>(dest);
	}
};

template<> std::string& MsgStreamReader::Helper<std::string>::Read();
template<> std::wstring& MsgStreamReader::Helper<std::wstring>::Read();