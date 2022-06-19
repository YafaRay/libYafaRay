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

class Flags
{
	public:
		constexpr Flags() = default;
		constexpr Flags(const Flags &flags) = default;
		constexpr Flags(unsigned int flags) : data_(flags) { } // NOLINT(google-explicit-constructor)
		constexpr explicit operator unsigned int() const { return data_; }
		constexpr bool hasAny(const Flags &f) const;
		constexpr bool hasAll(const Flags &f) const;
		static constexpr inline bool hasAny(const Flags &f_1, const Flags &f_2);
		static constexpr inline bool hasAll(const Flags &f_1, const Flags &f_2);
	private:
		unsigned int data_ = 0;
};

inline constexpr Flags operator | (const Flags &f_1, const Flags &f_2)
{
	return (static_cast<unsigned int>(f_1) | static_cast<unsigned int>(f_2));
}

inline Flags operator|=(Flags &f_1, const Flags &f_2)
{
	return f_1 = (f_1 | f_2);
}

inline constexpr Flags operator & (const Flags &f_1, const Flags &f_2)
{
	return (static_cast<unsigned int>(f_1) & static_cast<unsigned int>(f_2));
}

inline Flags operator&=(Flags &f_1, const Flags &f_2)
{
	return f_1 = (f_1 & f_2);
}

inline constexpr bool operator!(const Flags &f)
{
	return static_cast<unsigned int>(f) == 0;
}

inline constexpr bool operator == (const Flags &f_1, const Flags &f_2)
{
	return static_cast<unsigned int>(f_1) == static_cast<unsigned int>(f_2);
}

inline constexpr bool operator != (const Flags &f_1, const Flags &f_2)
{
	return !(f_1 == f_2);
}

inline constexpr bool Flags::hasAny(const Flags &f_1, const Flags &f_2)
{
	return !!(f_1 & f_2);
}

inline constexpr bool Flags::hasAny(const Flags &f_2) const
{
	return hasAny(*this, f_2);
}

inline constexpr bool Flags::hasAll(const Flags &f_1, const Flags &f_2)
{
	return !(f_1 & !f_2);
}

inline constexpr bool Flags::hasAll(const Flags &f_2) const
{
	return hasAll(*this, f_2);
}

} //namespace yafaray

#endif //YAFARAY_FLAGS_H
