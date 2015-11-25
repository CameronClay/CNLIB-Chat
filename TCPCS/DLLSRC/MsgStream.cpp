#include "CNLIB\MsgStream.h"

template<typename T> void StreamWriter::Helper<std::basic_string<T>>::Write(const std::basic_string<T>& t)
{
	const UINT len = t.size();
	stream.Write(len);
	stream.Write(t.c_str(), len * sizeof(T));
}
template<typename T> static UINT StreamWriter::SizeHelper<std::basic_string<T>>::SizeType(const std::basic_string<T>& t)
{
	return t.size() * sizeof(T);
}

template<typename T> std::basic_string<T>&& StreamReader::Helper<std::basic_string<T>>::Read()
{
	return stream.Read<T>(stream.Read<UINT>());
}

template<typename T> void StreamWriter::Helper<std::vector<T>>::Write(const std::vector<T>& t)
{
	const UINT size = t.size();
	stream.Write(size);
	for (auto& a : t)
	{
		stream.Write(a);
	}
}
template<typename T> static UINT StreamWriter::SizeHelper<std::vector<T>>::SizeType(const std::vector<T>& t)
{
	UINT size = MsgStreamWriter::SizeHelper<UINT>::SizeType();
	for (auto& a : t)
		size += MsgStreamWriter::SizeHelper<T>::SizeType(a);

	return size;
}

template<typename T> std::vector<T>&& StreamReader::Helper<std::vector<T>>::Read()
{
	const UINT size = stream.Read<UINT>();
	std::vector<T> temp(size);
	for (UINT i = 0; i < size; i++)
		temp.emplace(stream.Read<T>());

	return std::move(temp);
}



