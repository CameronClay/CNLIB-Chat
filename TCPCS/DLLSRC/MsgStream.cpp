#include "CNLIB\MsgStream.h"

void MsgStreamWriter::Helper<std::string>::Write(const std::string& t)
{
	const UINT len = (t.size() + 1);
	((Helper<UINT>*)this)->Write(len);
	((Helper<const char>*)this)->Write(t.c_str(), len * sizeof(char));
}
void MsgStreamWriter::Helper<std::wstring>::Write(const std::wstring& t)
{
	const UINT len = (t.size() + 1);
	((Helper<UINT>*)this)->Write(len);
	((Helper<const wchar_t>*)this)->Write(t.c_str(), len * sizeof(wchar_t));
}

std::string& MsgStreamReader::Helper<std::string>::Read()
{
	return ((Helper<char>*)this)->Read(((Helper<UINT>*)this)->Read());
}
std::wstring& MsgStreamReader::Helper<std::wstring>::Read()
{
	return ((Helper<wchar_t>*)this)->Read(((Helper<UINT>*)this)->Read());
}