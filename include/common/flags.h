#pragma once
/****************************************************************************
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

#ifndef YAFARAY_FLAGS_H
#define YAFARAY_FLAGS_H

#include <sstream>

namespace yafaray {

template<typename EnumClass>
inline constexpr EnumClass operator | (EnumClass f_1, EnumClass f_2)
{
	return static_cast<EnumClass>(static_cast<unsigned int>(f_1) | static_cast<unsigned int>(f_2));
}

template<typename EnumClass>
inline constexpr EnumClass operator & (EnumClass f_1, EnumClass f_2)
{
	return static_cast<EnumClass>(static_cast<unsigned int>(f_1) & static_cast<unsigned int>(f_2));
}

template<typename EnumClass>
inline EnumClass operator|=(EnumClass &f_1, EnumClass f_2)
{
	return f_1 = static_cast<EnumClass>(static_cast<unsigned int>(f_1) | static_cast<unsigned int>(f_2));
}

namespace flags
{

template<typename EnumClass>
inline bool have(EnumClass f_1, EnumClass f_2)
{
	return (static_cast<unsigned int>(f_1) & static_cast<unsigned int>(f_2)) != 0;
}

} //namespace flags
} //namespace yafaray

#endif //YAFARAY_FLAGS_H
