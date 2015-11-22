#include "CNLIB\MsgStream.h"

template<> void MsgStreamWriter::Helper<std::string>::Write(const std::string& t)
{
	const UINT len = t.size();
	stream.Write(len);
	stream.Write(t.c_str(), len * sizeof(char));
}
template<> void MsgStreamWriter::Helper<std::wstring>::Write(const std::wstring& t)
{
	const UINT len = t.size();
	stream.Write(len);
	stream.Write(t.c_str(), len * sizeof(wchar_t));
}

template<> void MsgStreamReader::Helper<std::string>::Read(std::string& t)
{
	t = stream.Read<char>(stream.Read<UINT>());
}
template<> void MsgStreamReader::Helper<std::wstring>::Read(std::wstring& t)
{
	t = stream.Read<wchar_t>(stream.Read<UINT>());
}