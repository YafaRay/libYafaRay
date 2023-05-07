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
#include "param/param.h"
#include "param/param_result.h"
#include "common/logger.h"
#include <vector>

namespace yafaray {

class ParamMap;

namespace class_meta {

#define PARAM_DECL(type, name, default_val, api_name, api_desc) inline static const ParamMeta name##meta_{api_name, api_desc, type{default_val}}; \
type name{name##meta_.getDefault<type>()}

#define PARAM_ENUM_DECL(enum_type, name, default_val, api_name, api_desc) inline static const ParamMeta name##meta_{api_name, api_desc, default_val, &enum_type::map_}; \
enum_type name{name##meta_.getDefault<enum_type::ValueType_t>()}

#define PARAM_META(param_name) param_meta_map[param_name##meta_.name()] = &param_name##meta_

#define PARAM_LOAD(param_name) if(param_map.getParam(param_name##meta_, param_name) == YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE) { \
param_result.flags_ |= ResultFlags{YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE}; \
param_result.wrong_type_params_.emplace_back(param_name##meta_.name()); }

#define PARAM_ENUM_LOAD(param_name) if(param_map.getEnumParam(param_name##meta_, param_name) == YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE) { \
param_result.flags_ |= ResultFlags{YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE}; \
param_result.wrong_type_params_.emplace_back(param_name##meta_.name()); }

#define PARAM_SAVE(name) if(!only_non_default || !Params::name##meta_.isDefault(params_.name)) param_map.setParam(Params::name##meta_, params_.name)

#define PARAM_SAVE_VARIABLE(name, variable) if(!only_non_default || !Params::name##meta_.isDefault(variable)) param_map.setParam(Params::name##meta_, variable)

#define PARAM_ENUM_SAVE(name) if(!only_non_default || !Params::name##meta_.isDefault(params_.name.value())) param_map.setParam(Params::name##meta_, params_.name.print())

#define CHECK_PARAM_NOT_DEFAULT(name) params_.name != Params::name##meta_.getDefault<decltype(params_.name)>()


template<typename TypeEnumClass>
inline TypeEnumClass getTypeFromParamMap(Logger &logger, const std::string &class_name, const ParamMap &param_map, bool allow_no_type = false)
{
	std::string type_str;
	ParamResult type_error{param_map.getParam("type", type_str)};
	TypeEnumClass type;
	if(!type.initFromString(type_str))
	{
		if(type_error.flags_ == YAFARAY_RESULT_WARNING_PARAM_NOT_SET && allow_no_type) return type;
		else type_error.flags_ |= YAFARAY_RESULT_WARNING_UNKNOWN_ENUM_OPTION;
	}
	if(type_error.notOk())
	{
		std::string warning_message{class_name + ": error in parameter 'type' (string): "};
		if(type_error.flags_.has(YAFARAY_RESULT_WARNING_PARAM_NOT_SET)) warning_message.append("It has not been set. ");
		if(type_error.flags_.has(YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE)) warning_message.append("It has been set with an incorrect type, it should be String. ");
		if(type_error.flags_.has(YAFARAY_RESULT_WARNING_UNKNOWN_ENUM_OPTION)) warning_message.append("The option '" + type_str + "' is unknown. ");
		warning_message.append("Valid types: \n" + TypeEnumClass::map_.print(TypeEnumClass::None));
		logger.logError(warning_message);
	}
	return type;
}

template<typename ParamsClass>
inline std::string print(const std::vector<std::string> &excluded_params)
{
	std::stringstream ss;
	const auto param_meta_map{ParamsClass::getParamMetaMap()};
	for(const auto &[var_name, var]: param_meta_map)
	{
		bool skip_param = false;
		for(const auto &excluded_param: excluded_params)
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
		static constexpr auto visit_to_string{[](auto&& val){
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
		ss << " " << std::visit(visit_to_string, var->defaultValue());
		ss << var->print();
	}
	return ss.str();
}

template<typename ParamsClass>
inline ParamResult check(const ParamMap &param_map, const std::vector<std::string> &excluded_params, const std::vector<std::string> &excluded_params_starting_with)
{
	ParamResult param_result;
	const auto param_meta_map{ParamsClass::getParamMetaMap()};
	for(const auto &[param_name, param]: param_map)
	{
		bool skip_param = false;
		for(const auto &excluded_param: excluded_params)
		{
			if(param_name == excluded_param)
			{
				skip_param = true;
				break;
			}
		}
		for(const auto &excluded_param_starting_with: excluded_params_starting_with)
		{
			if(param_name.rfind(excluded_param_starting_with, 0) == 0)
			{
				skip_param = true;
				break;
			}
		}
		if(skip_param) continue;
		static constexpr auto find{[](const std::string &name, const std::map<std::string, const ParamMeta *> &param_meta_map) -> const ParamMeta *
		{
			const auto &i{param_meta_map.find(name)};
			if(i != param_meta_map.end()) return i->second;
			else return nullptr;
		}};
		const auto param_meta{find(param_name, param_meta_map)};
		if(!param_meta)
		{
			param_result.flags_ |= ResultFlags{YAFARAY_RESULT_WARNING_UNKNOWN_PARAM};
			param_result.unknown_params_.emplace_back(param_name);
			continue;
		}
		else if(param_meta->isEnum())
		{
			std::string param_value_str;
			param.getVal(param_value_str);
			if(!param_meta->enumContains(param_value_str))
			{
				param_result.flags_ |= ResultFlags{YAFARAY_RESULT_WARNING_UNKNOWN_ENUM_OPTION};
				param_result.unknown_enum_.emplace_back(std::pair<std::string, std::string>{param_name, param_value_str});
				continue;
			}
		}
	}
	return param_result;
}

} //namespace class_meta

} //namespace yafaray

#endif //LIBYAFARAY_CLASS_META_H
