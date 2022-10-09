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

#ifndef LIBYAFARAY_ENUM_H
#define LIBYAFARAY_ENUM_H

#include <string>

namespace yafaray
{

template<typename EnumDerived, typename T=unsigned char>
class Enum
{
	public:
		constexpr Enum() = default;
		constexpr Enum(const Enum &an_enum) = default;
		constexpr Enum(Enum &&an_enum) = default;
		constexpr Enum(T value) { value_ = value; }
		explicit Enum(const std::string &str) { initFromString(str); }
		constexpr Enum& operator=(const Enum &an_enum) = default;
		constexpr Enum& operator=(Enum &&an_enum) = default;
		constexpr Enum& operator=(T value) { value_ = value; return *this; }
		//[[nodiscard]] constexpr operator T() const { return value_; } //Avoid (if possible) using implicit conversions, difficult to control and manage and can cause surprises...
		[[nodiscard]] constexpr T value() const { return value_; }
		[[nodiscard]] std::string print() const;
		[[nodiscard]] std::string printDescription() const;
		bool initFromValue(T value);
		bool initFromString(const std::string &str);
		[[nodiscard]] constexpr bool has(Enum an_enum) const;
		constexpr Enum &operator|=(Enum an_enum);
		constexpr bool operator==(Enum an_enum) const;
		constexpr bool operator!=(Enum an_enum) const;
		constexpr bool operator==(T value) const;
		constexpr bool operator!=(T value) const;
		using ValueType_t = T;

	private:
		T value_{0};
};

template<typename EnumDerived, typename T>
inline std::string Enum<EnumDerived, T>::print() const
{
	const auto result{EnumDerived::map_.find(value_)};
	return result ? result->first : "(?)";
}

template<typename EnumDerived, typename T>
inline std::string Enum<EnumDerived, T>::printDescription() const
{
	const auto result{EnumDerived::map_.find(value_)};
	return result ? result->second : "";
}

template<typename EnumDerived, typename T>
inline bool Enum<EnumDerived, T>::initFromValue(T value)
{
	const auto result{EnumDerived::map_.find(value)};
	if(result) value_ = value;
	return result;
}

template<typename EnumDerived, typename T>
inline bool Enum<EnumDerived, T>::initFromString(const std::string &str)
{
	const auto result{EnumDerived::map_.find(str)};
	if(result) value_ = result->first;
	return result;
}

template<typename EnumDerived, typename T>
inline std::ostream &operator<<(std::ostream &os, EnumDerived an_enum)
{
	os << an_enum.print();
	return os;
}

template <typename EnumDerived, typename T>
inline constexpr bool Enum<EnumDerived, T>::has(Enum an_enum) const
{
	return (value_ & an_enum.value_) != 0;
}

template <typename EnumDerived, typename T>
inline constexpr Enum<EnumDerived, T> &Enum<EnumDerived, T>::operator|=(Enum an_enum)
{
	value_ |= an_enum.value();
	return *this;
}

template <typename EnumDerived, typename T>
inline constexpr bool Enum<EnumDerived, T>::operator==(Enum an_enum) const
{
	return value_ == an_enum.value();
}

template <typename EnumDerived, typename T>
inline constexpr bool Enum<EnumDerived, T>::operator!=(Enum an_enum) const
{
	return value_ != an_enum.value();
}

template <typename EnumDerived, typename T>
inline constexpr bool Enum<EnumDerived, T>::operator==(T value) const
{
	return value_ == value;
}

template <typename EnumDerived, typename T>
inline constexpr bool Enum<EnumDerived, T>::operator!=(T value) const
{
	return value_ != value;
}

template<typename EnumDerived, typename T>
inline constexpr Enum<EnumDerived, T> operator|(Enum<EnumDerived, T> enum_1, Enum<EnumDerived, T> enum_2)
{
	return enum_1.value() | enum_2.value();
}

//Using int value because bitwise operators of unsigned char and unsigned short are implicitly converted to unsigned int
template<typename EnumDerived, typename T>
inline constexpr Enum<EnumDerived, T> operator|(unsigned int value, Enum<EnumDerived, T> an_enum)
{
	return value | an_enum.value();
}

//Using int value because bitwise operators of unsigned char and unsigned short are implicitly converted to unsigned int
template<typename EnumDerived, typename T>
inline constexpr Enum<EnumDerived, T> operator|(Enum<EnumDerived, T> an_enum, unsigned int value)
{
	return an_enum.value() | value;
}

template<typename EnumDerived, typename T>
inline constexpr Enum<EnumDerived, T> operator&(Enum<EnumDerived, T> enum_1, Enum<EnumDerived, T> enum_2)
{
	return enum_1.value() & enum_2.value();
}

//Using int value because bitwise operators of unsigned char and unsigned short are implicitly converted to unsigned int
template<typename EnumDerived, typename T>
inline constexpr Enum<EnumDerived, T> operator&(unsigned int value, Enum<EnumDerived, T> an_enum)
{
	return value & an_enum.value();
}

//Using int value because bitwise operators of unsigned char and unsigned short are implicitly converted to unsigned int
template<typename EnumDerived, typename T>
inline constexpr Enum<EnumDerived, T> operator&(Enum<EnumDerived, T> an_enum, unsigned int value)
{
	return an_enum.value() & value;
}

} //namespace yafaray

#endif //LIBYAFARAY_ENUM_H
