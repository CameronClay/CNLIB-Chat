#include "CNLIB\MsgStream.h"

//template<typename T> void MsgStreamWriter::Helper<std::vector<T>>::Write(const std::vector<T>& t)
//{
//
//}
//template<typename T> UINT MsgStreamWriter::Helper<std::vector<T>>::SizeType(const std::vector<T>& t) const
//{
//	UINT size = 0;
//	for (auto& a : t)
//	{
//		size += stream.SizeType(a);
//	}
//	return size;
//}

template<> void MsgStreamWriter::Helper<std::string>::Write(const std::string& t)
{
	const UINT len = t.size();
	stream.Write(len);
	stream.Write(t.c_str(), len * sizeof(char));
}
template<> UINT MsgStreamWriter::Helper<std::string>::SizeType(const std::string& t) const
{
	return t.size() * sizeof(char);
}

template<> void MsgStreamWriter::Helper<std::wstring>::Write(const std::wstring& t)
{
	const UINT len = t.size();
	stream.Write(len);
	stream.Write(t.c_str(), len * sizeof(wchar_t));
}
template<> UINT MsgStreamWriter::Helper<std::wstring>::SizeType(const std::wstring& t) const
{
	return t.size() * sizeof(wchar_t);
}


template<> void MsgStreamReader::Helper<std::string>::Read(std::string& t)
{
	t = stream.Read<char>(stream.Read<UINT>());
}

template<> void MsgStreamReader::Helper<std::wstring>::Read(std::wstring& t)
{
	t = stream.Read<wchar_t>(stream.Read<UINT>());
}