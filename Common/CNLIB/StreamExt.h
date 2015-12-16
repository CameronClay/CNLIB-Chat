template<typename T> 
class StreamWriter::Helper<std::basic_string<T>> : public HelpBase<std::basic_string<T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::basic_string<T>& t)
	{
		const UINT len = t.size();
		stream.Write(len);
		stream.Write(t.c_str(), len);
	}
	static UINT SizeType(const std::basic_string<T>& t)
	{
		return sizeof(size_t) + t.size() * sizeof(T);
	}
};

template<typename T> 
class StreamReader::Helper<std::basic_string<T>> : public HelpBase<std::basic_string<T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::basic_string<T> Read()
	{
		const size_t size = stream.Read<size_t>();
		return std::basic_string<T>(stream.Read<T>(size), size);
	}
};

template<typename T1, typename T2>
class StreamWriter::Helper<std::pair<T1, T2>> : public HelpBase<std::pair<T1, T2>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::pair<T1, T2>& t)
	{
		//Write in reverse order because of Read's order of eval
		stream << t.second << t.first;
	}
	static UINT SizeType(const std::pair<T1, T2>& t)
	{
		return Helper<T1>::SizeType(t.first) + Helper<T2>::SizeType(t.second);
	}
};

template<typename T1, typename T2>
class StreamReader::Helper<std::pair<T1, T2>> : public HelpBase<std::pair<T1, T2>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::pair<T1, T2> Read()
	{
		//Reads in reverse order because of order of eval
		return std::make_pair(stream.Read<T1>(), stream.Read<T2>());
	}
};

template<typename T> 
class StreamWriter::Helper<std::vector<T>> : public HelpBase<std::vector<T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::vector<T>& t)
	{
		stream.Write(t.size());
		for (auto& a : t)
			stream.Write(a);
	}
	static UINT SizeType(const std::vector<T>& t)
	{
		size_t size = sizeof(size_t);
		for (auto& a : t)
			size += Helper<T>::SizeType(a);

		return size;
	}
};

template<typename T> 
class StreamReader::Helper<std::vector<T>> : public HelpBase<std::vector<T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::vector<T> Read()
	{
		const size_t size = stream.Read<size_t>();
		std::vector<T> temp;
		temp.reserve(size);
		for (size_t i = 0; i < size; i++)
			temp.push_back(std::move(stream.Read<T>()));

		return temp;
	}
};

template<typename T>
class StreamWriter::Helper<std::list<T>> : public HelpBase<std::list<T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::list<T>& t)
	{
		stream.Write(t.size());
		for (auto& a : t)
			stream.Write(a);
	}
	static UINT SizeType(const std::list<T>& t)
	{
		size_t size = sizeof(size_t);
		for (auto& a : t)
			size += Helper<T>::SizeType(a);

		return size;
	}
};

template<typename T>
class StreamReader::Helper<std::list<T>> : public HelpBase<std::list<T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::list<T> Read()
	{
		const size_t size = stream.Read<size_t>();
		std::list<T> temp;
		for (UINT i = 0; i < size; i++)
			temp.push_back(std::move(stream.Read<T>()));

		return temp;
	}
};

template<typename T>
class StreamWriter::Helper<std::forward_list<T>> : public HelpBase<std::forward_list<T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::forward_list<T>& t)
	{
		const size_t size = std::distance(t.cbegin(), t.cend());
		stream.Write(size);
		for (auto& a : t)
			stream.Write(a);
	}
	static UINT SizeType(const std::forward_list<T>& t)
	{
		size_t size = sizeof(size_t);
		for (auto& a : t)
			size += Helper<T>::SizeType(a);

		return size;
	}
};

template<typename T>
class StreamReader::Helper<std::forward_list<T>> : public HelpBase<std::forward_list<T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::forward_list<T> Read()
	{
		const size_t size = stream.Read<size_t>();
		std::forward_list<T> temp;
		for (size_t i = 0; i < size; i++)
			temp.push_front(std::move(stream.Read<T>()));

		return temp;
	}
};

template<typename Key, typename T>
class StreamWriter::Helper<std::map<Key, T>> : public HelpBase<std::map<Key, T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::map<Key, T>& t)
	{
		stream.Write(t.size());
		for (auto& a : t)
			stream.Write(a);
	}
	static UINT SizeType(const std::map<Key, T>& t)
	{
		size_t size = sizeof(size_t);
		for (auto& a : t)
			size += Helper<std::pair<Key,T>>::SizeType(a);

		return size;
	}
};

template<typename Key, typename T>
class StreamReader::Helper<std::map<Key, T>> : public HelpBase<std::map<Key, T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::map<Key, T> Read()
	{
		const size_t size = stream.Read<size_t>();
		std::map<Key, T> temp;
		for (size_t i = 0; i < size; i++)
			temp.insert(std::move(stream.Read<std::pair<Key, T>>()));

		return temp;
	}
};

template<typename Key, typename T>
class StreamWriter::Helper<std::unordered_map<Key, T>> : public HelpBase<std::unordered_map<Key, T>>
{
public:
	Helper(StreamWriter& stream) : HelpBase(stream){}
	void Write(const std::unordered_map<Key, T>& t)
	{
		stream.Write(t.size());
		for (auto& a : t)
			stream.Write(a);
	}
	static UINT SizeType(const std::unordered_map<Key, T>& t)
	{
		size_t size = sizeof(size_t);
		for (auto& a : t)
			size += Helper<std::pair<Key,T>>::SizeType(a);

		return size;
	}
};

template<typename Key, typename T>
class StreamReader::Helper<std::unordered_map<Key, T>> : public HelpBase<std::unordered_map<Key, T>>
{
public:
	Helper(StreamReader& stream) : HelpBase(stream){}
	std::unordered_map<Key, T> Read()
	{
		const size_t size = stream.Read<size_t>();
		std::unordered_map<Key, T> temp;
		temp.reserve(size);
		for (UINT i = 0; i < size; i++)
			temp.insert(std::move(stream.Read<std::pair<Key, T>>()));

		return temp;
	}
};
