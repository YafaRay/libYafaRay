#pragma once
/****************************************************************************
 *
 *      This is part of the libYafaRay package
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

#ifndef LIBYAFARAY_ENUM_MAP_H
#define LIBYAFARAY_ENUM_MAP_H

#include "common/bimap.h"
#include <string>
#include <sstream>

namespace yafaray {

template <typename T>
class EnumMap : public BiMap<std::string, T, std::string>
{
	public:
		[[nodiscard]] std::string print(T default_value) const;
};

template <typename T>
inline std::string EnumMap<T>::print(T default_value) const
{
	std::stringstream ss;
	for(const auto &[key, value] : *this)
	{
		ss << "  - '" << key << "'";
		if(!value.second.empty()) ss << " [" << value.second << "]";
		if(value.first == default_value) ss << " (default)";
		ss << std::endl;
	}
	return ss.str();
}

} //namespace yafaray

#endif //LIBYAFARAY_ENUM_MAP_H
