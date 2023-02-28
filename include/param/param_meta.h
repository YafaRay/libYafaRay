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

#ifndef LIBYAFARAY_PARAM_META_H
#define LIBYAFARAY_PARAM_META_H

#include "common/enum_map.h"
#include "geometry/vector.h"
#include "geometry/matrix.h"
#include "color/color.h"
#include <string>
#include <variant>

namespace yafaray {

class ParamMeta
{
	public:
		ParamMeta() = default;
		template<typename T> ParamMeta(std::string name, std::string desc, T default_value);
		ParamMeta(std::string name, std::string desc, unsigned char default_value, const EnumMap<unsigned char> *enum_map);
		template <typename T> [[nodiscard]] T getDefault() const { return std::get<T>(default_value_); }
		template <typename T> [[nodiscard]] bool isDefault(const T &value) const { return value == std::get<T>(default_value_); }
		[[nodiscard]] std::string name() const { return name_; }
		[[nodiscard]] std::string desc() const { return desc_; }
		[[nodiscard]] std::variant<bool, int, float, double, unsigned char, std::string, Vec3f, Rgb, Rgba, Matrix4f> defaultValue() const { return default_value_; }
		[[nodiscard]] std::string print() const;
		[[nodiscard]] bool isEnum() const { return map_ != nullptr;}
		[[nodiscard]] bool enumContains(const std::string &str) const { return map_ ? (map_->find(str) != nullptr) : false;}
		template <typename EnumType> [[nodiscard]] static std::array<ParamMeta, EnumType::Size> enumToParamMetaArray();

	private:
		std::string name_;
		std::string desc_;
		std::variant<bool, int, float, double, unsigned char, std::string, Vec3f, Rgb, Rgba, Matrix4f> default_value_;
		const EnumMap<unsigned char> *map_ = nullptr;
};

template<typename T>
inline ParamMeta::ParamMeta(std::string name, std::string desc, T default_value) : name_{std::move(name)}, desc_{std::move(desc)}, default_value_{std::move(default_value)}
{
	static_assert(!std::is_same<T, char>::value, "Enums (char type) cannot be constructed without specifying the EnumMap pointer");
}

inline ParamMeta::ParamMeta(std::string name, std::string desc, unsigned char default_value, const EnumMap<unsigned char> *enum_map) : name_{std::move(name)}, desc_{std::move(desc)}, default_value_{default_value}, map_{enum_map}
{
}

inline std::string ParamMeta::print() const
{
	static constexpr auto getEnumDefaultValue{[](auto&& val){
		using Type = std::decay_t<decltype(val)>;
		//Disabling actual visiting, for now only using unsigned char for parameter enums
		//if constexpr (std::is_same_v<Type, unsigned char>) return std::get<unsigned char>(val);
		//else if(std::is_same_v<Type, unsigned short>) return std::get<unsigned short>(val);
		//else return static_cast<unsigned short>(0);
		return std::get<unsigned char>(val);
	}};
	std::stringstream ss;
	ss << std::endl;
	if(map_) ss << map_->print(getEnumDefaultValue(default_value_));
	return ss.str();
}

template <typename EnumType>
inline std::array<ParamMeta, EnumType::Size> ParamMeta::enumToParamMetaArray()
{
	std::array<ParamMeta, EnumType::Size> result;
	for(size_t index = 0; index < EnumType::Size; ++index)
	{
		const auto enum_entry{EnumType{static_cast<unsigned char>(index)}};
		const std::string param_name{enum_entry.print()};
		const std::string param_desc{enum_entry.printDescription()};
		result[index] = ParamMeta{param_name, param_desc, std::string{}};
	}
	return result;
}

} //namespace yafaray

#endif //LIBYAFARAY_PARAM_META_H
