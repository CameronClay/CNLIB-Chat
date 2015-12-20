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

		if(size() < max_size())
			++written;
	}

	void push_back(T&& val)
	{
		if(_end == _endPtr)
			_end = _data - 1;

		*++_end = val;

		if(size() < max_size())
			++written;
	}

	void push_back_ninc(const T& val)
	{
		if(_end == _endPtr)
			_end = _data - 1;

		*++_end = val;
	}

	void push_back_ninc(T&& val)
	{
		if(_end == _endPtr)
			_end = _data - 1;

		*++_end = val;
	}

	void IncreaseWritten(size_t amount)
	{
		written += min(amount, max_size() - written);
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

	void pop_back()
	{
		if(written)
		{
			--written;
			*_end = std::forward<T>(T());
			if(_end + 1 == _data)
				_end = (T*)_endPtr;
			else
				--_end;
		}
	}

	void erase(size_t count)
	{
		count = min(written, count);

		for(size_t i = 0; i < count; i++)
		{
			*_begin = std::forward<T>(T());
			if(_begin == _endPtr)
				_begin = _data;
			else
				++_begin;
		}

		written -= count;
	}

	const T& front() const 
	{
		return *_begin;
	}

	const T& back() const 
	{
		return *_end;
	}

	const T& operator[](size_t i) const
	{
		if(_begin + i > _endPtr)
			return *(_data + ((_begin + i) - (_endPtr + 1)));

		return *(_begin + i);
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
