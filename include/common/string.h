#pragma once
/****************************************************************************
 *
 *      stringUtils.h: Some string manipulation utilities
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez (DarkTide)
 *      Creation date: 2010-05-06
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef YAFARAY_STRING_H
#define YAFARAY_STRING_H

#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <locale>
#include <codecvt>

BEGIN_YAFARAY

template <class T>
inline void converter__(std::string str, T &val)
{
	std::stringstream conv;

	conv << str;
	conv >> std::skipws >> val;
}

inline std::string toLower__(const std::string &in)
{
	std::string out = in;

	for(size_t i = 0; i < in.length(); i++)
	{
		out[i] = std::tolower(in[i]);
	}

	return out;
}

inline std::vector<std::string> tokenize__(std::string str, std::string delimiter = " ")
{
	std::vector<std::string> result;
	size_t last_pos = str.find_first_not_of(delimiter, 0);
	size_t pos = str.find_first_of(delimiter, last_pos);

	while(std::string::npos != pos || std::string::npos != last_pos)
	{
		result.push_back(str.substr(last_pos, pos - last_pos));
		last_pos = str.find_first_not_of(delimiter, pos);
		pos = str.find_first_of(delimiter, last_pos);
	}

	return result;
}

inline std::u32string utf8ToWutf32__(const std::string &utf_8_str)
{
#if defined(_MSC_VER) && _MSC_VER >= 1900
	//Workaround for bug in VS2015/2017
	//https://stackoverflow.com/questions/32055357/visual-studio-c-2015-stdcodecvt-with-char16-t-or-char32-t
	std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> string_conversion;
	return reinterpret_cast<const char32_t *>(string_conversion.from_bytes(utf_8_str).data());
#else
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> string_conversion;
	return string_conversion.from_bytes(utf_8_str);
#endif
}

inline std::string wutf32ToUtf8__(const std::u32string &wutf_32_str)
{
#if defined(_MSC_VER) && _MSC_VER >= 1900
	//Workaround for bug in VS2015/2017
	//https://stackoverflow.com/questions/32055357/visual-studio-c-2015-stdcodecvt-with-char16-t-or-char32-t
	std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> string_conversion;
	auto p = reinterpret_cast<const int32_t *>(wutf_32_str.data());
	return string_conversion.to_bytes(p, p + wutf_32_str.size());
#else
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> string_conversion;
	return string_conversion.to_bytes(wutf_32_str);
#endif
}

inline std::wstring utf8ToWutf16Le__(const std::string &utf_8_str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10FFFF, std::little_endian>, wchar_t> string_conversion;
	return string_conversion.from_bytes(utf_8_str);
}

inline std::string wutf16LeToUtf8__(const std::wstring &wutf_16_str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10FFFF, std::little_endian>, wchar_t> string_conversion;
	return string_conversion.to_bytes(wutf_16_str);
}

END_YAFARAY

#endif
