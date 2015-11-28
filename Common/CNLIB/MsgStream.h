//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "HeapAlloc.h"
#include <assert.h>
#include <array>
#include <string>
#include <vector>
#include <list>
#include <forward_list>
#include <map>
#include <unordered_map>

#define MSG_OFFSET 2

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
		data(stream.data),
		capacity(stream.capacity)
	{
		ZeroMemory(&stream, sizeof(MsgStream));
	}
/*
	~MsgStream()
	{
		dealloc((char*&)begin);
	}
*/
	char GetType() const
	{
		return data[-2];
	}
	char GetMsg() const
	{
		return data[-1];
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

	template<typename T> std::enable_if_t<std::is_arithmetic<T>::value, T> operator[](UINT index) const
	{
		assert(index <= capacity - 2);
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

typedef class MsgStreamWriter : public MsgStream
{
public:
	//capacity not including MSG_OFFSET
	MsgStreamWriter(char type, char msg, UINT capacity)
		:
		MsgStream(alloc<char>(capacity + MSG_OFFSET), capacity)
	{
		data[-2] = type;
		data[-1] = msg;
	}
	MsgStreamWriter(char type, char msg, char* ptr, UINT nBytes)
		:
		MsgStream(ptr, nBytes)
	{
		data[-2] = type;
		data[-1] = msg;
	}
	MsgStreamWriter(MsgStreamWriter&& stream)
		:
		MsgStream(std::move(stream))
	{}

	~MsgStreamWriter()
	{
		dealloc( (char*&)begin ); // would rather user handle allocation
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
		return Helper<T>(*this).operator<<(t);
	}

	template<typename T>
	typename std::enable_if_t<std::is_arithmetic<T>::value> Write(T* t, UINT count)
	{
		Helper<T>(*this).Write(t, count);
	}
	template<typename T>
	typename std::enable_if_t<std::is_arithmetic<T>::value> WriteEnd(T* t)
	{
		Helper<T>(*this).WriteEnd(t);
	}

	template<typename T>
	static std::enable_if_t<std::is_arithmetic<T>::value, UINT> SizeType()
	{
		return SizeHelper<T>::SizeType();
	}
	template<typename T>
	static UINT SizeType(const T& t)
	{
		return SizeHelper<T>::SizeType(t);
	}
private:
	template<typename T> struct SizeHelper
	{
		static UINT SizeType()
		{
			return sizeof(T);
		}
		static UINT SizeType(const T&)
		{
			return sizeof(T);
		}
	};
	template<typename T> class HelpBase
	{
	public:
		HelpBase(MsgStreamWriter& stream)
			:
			stream(stream)
		{}

		MsgStreamWriter& operator<<(const T& t)
		{
			stream.Write<T>(t);
			return stream;
		}
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
			const UINT nBytes = stream.end - stream.data;
			memcpy(stream.data, t, nBytes);
			stream.data += nBytes;
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

	template<typename T> T&& Read()
	{
		return Helper<T>(*this).Read();
	}
	template<typename T> void Read(T& t)
	{
		Helper<T>(*this).Read(t);
	}
	template<typename T> MsgStreamReader& operator>>(T& dest)
	{
		return Helper<T>(*this).operator>>(dest);
	}

	template<typename T> std::enable_if_t<std::is_arithmetic<T>::value, T>* Read(UINT count)
	{
		return Helper<T>(*this).Read(count);
	}
	template<typename T> std::enable_if_t<std::is_arithmetic<T>::value, T>* ReadEnd()
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

		void Read(T& t)
		{
			t = std::move(stream.Read<T>());
		}

		MsgStreamReader& operator>>(T& dest)
		{
			Read(dest);
			return stream;
		}
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

		T&& Read()
		{
			T t = *(T*)(stream.data);
			stream.data += sizeof(T);
			assert(stream.begin <= stream.end);

			return std::move(t);
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
			const UINT nBytes = stream.end - stream.data;
			T* t = (T*)(stream.data);
			stream.data += nBytes;

			return t;
		}
	};
} StreamReader;

template<typename T> 
class StreamWriter::Helper<std::basic_string<T>> : public HelpBase<std::basic_string<T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::basic_string<T>& t)
	{
		const UINT len = t.size();
		stream.Write(len);
		stream.Write(t.c_str(), len * sizeof(T));
	}
};

template<typename T> 
struct StreamWriter::SizeHelper<std::basic_string<T>>
{
	static UINT SizeType(const std::basic_string<T>& t)
	{
		return t.size() * sizeof(T);
	}
};

template<typename T> 
class StreamReader::Helper<std::basic_string<T>> : public HelpBase<std::basic_string<T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::basic_string<T>&& Read()
	{
		return std::move(std::basic_string<T>(stream.Read<T>(stream.Read<UINT>())));
	}
};

template<typename T1, typename T2>
class StreamWriter::Helper<std::pair<T1, T2>> : public HelpBase<std::pair<T1, T2>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::pair<T1, T2>& t)
	{
		//Read in reverse order because of order of eval
		stream << t.second << t.first;
	}
};

template<typename T1, typename T2>
struct StreamWriter::SizeHelper<std::pair<T1, T2>>
{
	static UINT SizeType(const std::pair<T1, T2>& t)
	{
		return StreamWriter::SizeHelper<T1>::SizeType() +
			StreamWriter::SizeHelper<T2>::SizeType();
	}
};

template<typename T1, typename T2>
class StreamReader::Helper<std::pair<T1, T2>> : public HelpBase<std::pair<T1, T2>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::pair<T1, T2>&& Read()
	{
		//Read in reverse order because of order of eval
		return std::move(std::make_pair(stream.Read<T1>(), stream.Read<T2>()));
	}
};

template<typename T> 
class StreamWriter::Helper<std::vector<T>> : public HelpBase<std::vector<T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::vector<T>& t)
	{
		const UINT size = t.size();
		stream.Write(size);
		for (auto& a : t)
			stream.Write(a);
	}
};

template<typename T> 
struct StreamWriter::SizeHelper<std::vector<T>>
{
	static UINT SizeType(const std::vector<T>& t)
	{
		UINT size = StreamWriter::SizeHelper<UINT>::SizeType();
		for (auto& a : t)
			size += StreamWriter::SizeHelper<T>::SizeType(a);

		return size;
	}
};

template<typename T> 
class StreamReader::Helper<std::vector<T>> : public HelpBase<std::vector<T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::vector<T>&& Read()
	{
		const UINT size = stream.Read<UINT>();
		std::vector<T> temp(size);
		for (UINT i = 0; i < size; i++)
			temp.push_back(std::move(stream.Read<T>()));

		return std::move(temp);
	}
};

template<typename T>
class StreamWriter::Helper<std::list<T>> : public HelpBase<std::list<T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::list<T>& t)
	{
		const UINT size = t.size();
		stream.Write(size);
		for (auto& a : t)
			stream.Write(a);
	}
};

template<typename T>
struct StreamWriter::SizeHelper<std::list<T>>
{
	static UINT SizeType(const std::list<T>& t)
	{
		UINT size = StreamWriter::SizeHelper<UINT>::SizeType();
		for (auto& a : t)
			size += StreamWriter::SizeHelper<T>::SizeType(a);

		return size;
	}
};

template<typename T>
class StreamReader::Helper<std::list<T>> : public HelpBase<std::list<T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::list<T>&& Read()
	{
		const UINT size = stream.Read<UINT>();
		std::list<T> temp(size);
		for (UINT i = 0; i < size; i++)
			temp.push_back(std::move(stream.Read<T>()));

		return std::move(temp);
	}
};

template<typename T>
class StreamWriter::Helper<std::forward_list<T>> : public HelpBase<std::forward_list<T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::forward_list<T>& t)
	{
		const UINT size = t.size();
		stream.Write(size);
		for (auto& a : t)
			stream.Write(a);
	}
};

template<typename T>
struct StreamWriter::SizeHelper<std::forward_list<T>>
{
	static UINT SizeType(const std::forward_list<T>& t)
	{
		UINT size = StreamWriter::SizeHelper<UINT>::SizeType();
		for (auto& a : t)
			size += StreamWriter::SizeHelper<T>::SizeType(a);

		return size;
	}
};

template<typename T>
class StreamReader::Helper<std::forward_list<T>> : public HelpBase<std::forward_list<T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::forward_list<T>&& Read()
	{
		const UINT size = stream.Read<UINT>();
		std::forward_list<T><T> temp(size);
		for (UINT i = 0; i < size; i++)
			temp.push_back(std::move(stream.Read<T>()));

		return std::move(temp);
	}
};

template<typename Key, typename T>
class StreamWriter::Helper<std::map<Key, T>> : public HelpBase<std::map<Key, T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::map<Key, T>& t)
	{
		const UINT size = t.size();
		stream.Write(size);
		for (auto& a : t)
		{
			stream.Write(a);
		}
	}
};

template<typename Key, typename T>
struct StreamWriter::SizeHelper<std::map<Key, T>>
{
	static UINT SizeType(const std::map<Key, T>& t)
	{
		UINT size = MsgStreamWriter::SizeHelper<UINT>::SizeType();
		for (auto& a : t)
			size += MsgStreamWriter::SizeHelper<std::pair<Key,T>>::SizeType(a);

		return size;
	}
};

template<typename Key, typename T>
class StreamReader::Helper<std::map<Key, T>> : public HelpBase<std::map<Key, T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::map<Key, T>&& Read()
	{
		const UINT size = stream.Read<UINT>();
		std::map<Key, T> temp(size);
		for (UINT i = 0; i < size; i++)
			temp.insert(std::move(stream.Read<T>()));

		return std::move(temp);
	}
};

template<typename Key, typename T>
class StreamWriter::Helper<std::unordered_map<Key, T>> : public HelpBase<std::unordered_map<Key, T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::unordered_map<Key, T>& t)
	{
		const UINT size = t.size();
		stream.Write(size);
		for (auto& a : t)
		{
			stream.Write(a);
		}
	}
};

template<typename Key, typename T>
struct StreamWriter::SizeHelper<std::unordered_map<Key, T>>
{
	static UINT SizeType(const std::unordered_map<Key, T>& t)
	{
		UINT size = MsgStreamWriter::SizeHelper<UINT>::SizeType();
		for (auto& a : t)
			size += MsgStreamWriter::SizeHelper<std::pair<Key,T>>::SizeType(a);

		return size;
	}
};

template<typename Key, typename T>
class StreamReader::Helper<std::unordered_map<Key, T>> : public HelpBase<std::unordered_map<Key, T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::unordered_map<Key, T>&& Read()
	{
		const UINT size = stream.Read<UINT>();
		std::unordered_map<Key, T> temp(size);
		for (UINT i = 0; i < size; i++)
			temp.insert(std::move(stream.Read<T>()));

		return std::move(temp);
	}
};
