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

#ifndef LIBYAFARAY_CLASS_META_H
#define LIBYAFARAY_CLASS_META_H

#include "param/param_meta.h"
#include "param/param_error.h"
#include "param/param.h"
#include "common/logger.h"
#include <vector>

namespace yafaray {

class ParamMap;

#define PARAM_INIT Params() = default; \
Params(ParamError &param_error, const ParamMap &param_map); \
ParamMap getAsParamMap(bool only_non_default) const; \
inline static ClassMeta meta_

#define PARAM_INIT_PARENT(parent_class) Params(ParamError &param_error, const ParamMap &param_map); \
ParamMap getAsParamMap(bool only_non_default) const; \
inline static ClassMeta meta_{parent_class::Params::meta_};

#define PARAM_DECL(type, name, default_val, api_name, api_desc) type name{name##meta_.getDefault<type>()}; \
inline static const ParamMeta name##meta_{api_name, api_desc, type{default_val}, meta_.map_}

#define PARAM_ENUM_DECL(enum_type, name, default_val, api_name, api_desc) enum_type name{name##meta_.getDefault<enum_type::ValueType_t>()}; \
inline static const ParamMeta name##meta_{api_name, api_desc, default_val, meta_.map_, &enum_type::map_}

#define PARAM_SHADERS_DECL inline static const std::array<std::unique_ptr<ParamMeta>, ShaderNodeType::Size> shader_node_names_meta_{initShaderNames<ShaderNodeType>(meta_.map_)}; \
std::array<std::string, ShaderNodeType::Size> shader_node_names_

#define PARAM_LOAD(param_name) if(param_map.getParam(param_name##meta_, param_name) == ParamError::Flags::ErrorWrongParamType) { \
param_error.flags_ |= ParamError::Flags{ParamError::Flags::ErrorWrongParamType}; \
param_error.wrong_type_params_.emplace_back(param_name##meta_.name()); }

#define PARAM_ENUM_LOAD(param_name) if(param_map.getEnumParam(param_name##meta_, param_name) == ParamError::Flags::ErrorWrongParamType) { \
param_error.flags_ |= ParamError::Flags{ParamError::Flags::ErrorWrongParamType}; \
param_error.wrong_type_params_.emplace_back(param_name##meta_.name()); }

#define PARAM_SHADERS_LOAD for(size_t index = 0; index < shader_node_names_.size(); ++index) { \
const std::string shader_node_type_name{ShaderNodeType{static_cast<unsigned char>(index)}.print()}; \
if(param_map.getParam(shader_node_type_name, shader_node_names_[index]) == ParamError::Flags::ErrorWrongParamType) { \
	param_error.flags_ |= ParamError::Flags{ParamError::Flags::ErrorWrongParamType}; \
	param_error.wrong_type_params_.emplace_back(shader_node_type_name); \
}}

#define PARAM_SAVE_START ParamMap param_map
#define PARAM_SAVE(name) if(!only_non_default || !name##meta_.isDefault(name)) param_map.setParam(name##meta_, name)
#define PARAM_ENUM_SAVE(name) if(!only_non_default || !name##meta_.isDefault(name.value())) param_map.setParam(name##meta_, name.print())

#define PARAM_SHADERS_SAVE for(size_t index = 0; index < shader_node_names_.size(); ++index) { \
const std::string shader_node_type_name{ShaderNodeType{static_cast<unsigned char>(index)}.print()}; \
if(!only_non_default || !shader_node_names_[index].empty()) param_map.setParam(shader_node_type_name, shader_node_names_[index]); }

#define PARAM_SAVE_END return param_map

#define CHECK_PARAM_NOT_DEFAULT(name) params_.name != Params::name##meta_.getDefault<decltype(params_.name)>()

struct ClassMeta
{
	[[nodiscard]] const ParamMeta *find(const std::string &name) const;
	[[nodiscard]] std::string print(const std::vector<std::string> &excluded_params) const;
	[[nodiscard]] ParamError check(const ParamMap &param_map, const std::vector<std::string> &excluded_params, const std::vector<std::string> &excluded_params_starting_with) const;
	template <typename TypeEnumClass> [[nodiscard]] static TypeEnumClass preprocessParamMap(Logger &logger, const std::string &class_name, const ParamMap &param_map);
	std::map<std::string, const ParamMeta *> map_;
};

inline const ParamMeta *ClassMeta::find(const std::string &name) const
{
	const auto &i{map_.find(name)};
	if(i != map_.end()) return i->second;
	else return nullptr;
}

inline std::string ClassMeta::print(const std::vector<std::string> &excluded_params) const
{
	static constexpr auto visitToString{[](auto&& val){
		using Type = std::decay_t<decltype(val)>;
		if constexpr (std::is_same_v<Type, bool>) return std::string{"(bool) default="} + (val ? "true" : "false");
		else if constexpr (std::is_same_v<Type, int>) return std::string{"(int) default="} + std::to_string(val);
		else if constexpr (std::is_same_v<Type, float>) return std::string{"(float) default="} + std::to_string(val);
		else if constexpr (std::is_same_v<Type, double>) return std::string{"(double) default="} + std::to_string(val);
		else if constexpr (std::is_same_v<Type, unsigned char> || std::is_same_v<Type, unsigned short>) return std::string{"(enum)"};
		else if constexpr (std::is_same_v<Type, std::string>) return std::string{"(string) default='"} + val + "'";
		else if constexpr (std::is_same_v<Type, Vec3f>) return std::string{"(vector) default=<x="} + std::to_string(val[0]) + ", y=" + std::to_string(val[1]) + ", z=" + std::to_string(val[2]) + ">";
		else if constexpr (std::is_same_v<Type, Rgb>) return std::string{"(color) default=<r="} + std::to_string(val.r_) + ", g=" + std::to_string(val.g_) + ", b=" + std::to_string(val.b_) + ">";
		else if constexpr (std::is_same_v<Type, Rgba>) return std::string{"(color + alpha) default=<r="} + std::to_string(val.r_) + ", g=" + std::to_string(val.g_) + ", b=" + std::to_string(val.b_) + ", a=" + std::to_string(val.a_) + ">";
		else if constexpr (std::is_same_v<Type, Matrix4f>)
		{
			std::stringstream ss;
			ss << "(matrix4) default=<" << val << ">";
			return ss.str();
		}
		else return std::string{"(*unknown*)"};
	}};
	std::stringstream ss;
	for(const auto &[var_name, var] : map_)
	{
		bool skip_param = false;
		for(const auto &excluded_param : excluded_params)
		{
			if(var_name == excluded_param)
			{
				skip_param = true;
				break;
			}
		}
		if(skip_param) continue;
		ss << "'" << var_name << "'";
		if(!var->desc().empty()) ss << " [" << var->desc() << "]";
		ss << " " << std::visit(visitToString, var->defaultValue());
		ss << var->print();
	}
	return ss.str();
}

template <typename TypeEnumClass>
inline TypeEnumClass ClassMeta::preprocessParamMap(Logger &logger, const std::string &class_name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + class_name + "::factory 'raw' ParamMap\n" + param_map.logContents());
	std::string type_str;
	ParamError type_error{param_map.getParam("type", type_str)};
	TypeEnumClass type;
	if(!type.initFromString(type_str)) type_error.flags_ |= ParamError::Flags::WarningUnknownEnumOption;
	if(type_error.notOk())
	{
		std::string warning_message{class_name + ": error in parameter 'type' (string): "};
		if(type_error.flags_.has(ParamError::Flags::WarningParamNotSet)) warning_message.append("It has not been set. ");
		if(type_error.flags_.has(ParamError::Flags::ErrorWrongParamType)) warning_message.append("It has been set with an incorrect type, it should be String. ");
		if(type_error.flags_.has(ParamError::Flags::WarningUnknownEnumOption)) warning_message.append("The option '" + type_str + "' is unknown. ");
		warning_message.append("Valid types: \n" + TypeEnumClass::map_.print(TypeEnumClass::None));
		logger.logError(warning_message);
	}
	return type;
}

} //namespace yafaray

#endif //LIBYAFARAY_CLASS_META_H
