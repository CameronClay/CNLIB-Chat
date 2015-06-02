#pragma once
#include "CNLIB\HeapAlloc.h"

template<typename T> class CircularBuffer
{
public:
	CircularBuffer(size_t size)
		:
		_data(alloc<T>(size)),
		_begin(_data),
		_end(_data - 1),
		_endPtr(_data + (size - 1)),
		written(0)
	{}	
	CircularBuffer(CircularBuffer&& buffer)
		:
		_data(buffer._data),
		_begin(buffer._begin),
		_end(buffer._end),
		_endPtr(buffer._endPtr),
		written(buffer.written)
	{
		ZeroMemory(&buffer, sizeof(CircularBuffer));
	}

	~CircularBuffer()
	{
		clear();
		dealloc(_data);
	}

	void push_back(const T& val)
	{
		if(_end == _endPtr)
			_end = _data - 1;

		*++_end = val;

		if(size() != max_size())
			++written;
	}

	void push_back(T&& val)
	{
		if(_end == _endPtr)
			_end = _data - 1;

		*++_end = val;

		if(size() != max_size())
			++written;
	}

	void pop_front()
	{
		if(written)
		{
			--written;
			*_begin = std::forward<T>(T());
			if(_begin == _endPtr)
				_begin = _data;
			else
				++_begin;
		}
	}

	const T& front()
	{
		return *_begin;
	}

	void clear()
	{
		if(_data)
		{
			ZeroMemory(_data, sizeof(T) * max_size());
			_begin = _data;
			_end = _data - 1;
			written = 0;
		}
	}

	inline size_t size() const
	{
		return written;
	}
	inline size_t max_size() const
	{
		return (_endPtr + 1) - _data;
	}
	inline size_t capacity() const
	{
		return max_size() * sizeof(T);
	}
	inline bool empty() const
	{
		return written == 0;
	}

private:
	T *_data, *_begin, *_end;
	const T *_endPtr;
	size_t written;
};