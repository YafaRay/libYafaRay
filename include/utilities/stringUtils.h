/****************************************************************************
 *
 *      stringUtils.h: Some std::string manipulation utilities
 *      This is part of the yafray package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez (DarkTide)
 *		Creation date: 2010-05-06
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

#ifndef Y_STRINGUTILS_H
#define Y_STRINGUTILS_H

#include <string>
#include <sstream>
#include <cctype>
#include <boost/locale/encoding_utf.hpp>

__BEGIN_YAFRAY

template <class T>
inline void converter(std::string str, T &val)
{
	std::stringstream conv;
	
	conv << str;
	conv >> std::skipws >> val;
}

inline std::string toLower(const std::string &in)
{
	std::string out = in;
	
	for(size_t i = 0; i < in.length(); i++)
	{
		out[i] = std::tolower(in[i]);
	}
	
	return out;
}

inline std::vector<std::string> tokenize(std::string str, std::string delimiter = " ")
{
	std::vector<std::string> result;
	size_t lastPos = str.find_first_not_of(delimiter, 0);
	size_t pos = str.find_first_of(delimiter, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos)
	{
		result.push_back(str.substr(lastPos, pos - lastPos));
		lastPos = str.find_first_not_of(delimiter, pos);
		pos = str.find_first_of(delimiter, lastPos);
	}
	
	return result;
}

inline std::u32string utf8_to_wutf32(const std::string& utf8str)
{
    return boost::locale::conv::utf_to_utf<char32_t>(utf8str.c_str(), utf8str.c_str() + utf8str.size());
}

inline std::string wutf32_to_utf8(const std::u32string& wutf32str)
{
    return boost::locale::conv::utf_to_utf<char>(wutf32str.c_str(), wutf32str.c_str() + wutf32str.size());
}  

inline std::wstring utf8_to_wutf16(const std::string& utf8str)
{
    return boost::locale::conv::utf_to_utf<wchar_t>(utf8str.c_str(), utf8str.c_str() + utf8str.size());
}

inline std::string wutf16_to_utf8(const std::wstring& wutf16str)
{
    return boost::locale::conv::utf_to_utf<char>(wutf16str.c_str(), wutf16str.c_str() + wutf16str.size());
}  

__END_YAFRAY

#endif
