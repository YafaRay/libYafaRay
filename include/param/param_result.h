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

#ifndef LIBYAFARAY_PARAM_RESULT_H
#define LIBYAFARAY_PARAM_RESULT_H

#include "common/result_flags.h"

namespace yafaray {

struct ParamResult
{
	template<typename T> [[nodiscard]] std::string print(const std::string &name, const std::vector<std::string> &excluded_params) const;
	[[nodiscard]] bool isOk() const { return flags_.isOk(); }
	[[nodiscard]] bool notOk() const { return flags_.notOk(); }
	[[nodiscard]] bool hasError() const { return flags_.hasError(); }
	[[nodiscard]] bool hasWarning() const { return flags_.hasWarning(); }
	void merge(const ParamResult &param_result);
	ResultFlags flags_{YAFARAY_RESULT_OK};
	std::vector<std::string> unknown_params_{};
	std::vector<std::string> wrong_type_params_{};
	std::vector<std::pair<std::string, std::string>> unknown_enum_{};
};

inline void ParamResult::merge(const ParamResult &param_result)
{
	flags_ |= param_result.flags_;
	unknown_params_.insert(unknown_params_.end(), param_result.unknown_params_.begin(), param_result.unknown_params_.end());
	std::sort(unknown_params_.begin(), unknown_params_.end());
	wrong_type_params_.insert(wrong_type_params_.end(), param_result.wrong_type_params_.begin(), param_result.wrong_type_params_.end());
	std::sort(wrong_type_params_.begin(), wrong_type_params_.end());
	unknown_enum_.insert(unknown_enum_.end(), param_result.unknown_enum_.begin(), param_result.unknown_enum_.end());
	std::sort(unknown_enum_.begin(), unknown_enum_.end());
}

template <typename T>
inline std::string ParamResult::print(const std::string &name, const std::vector<std::string> &excluded_params) const
{
	std::stringstream ss;
	ss << T::getClassName() << " '" + name + "':";
	if(!unknown_params_.empty())
	{
		ss << std::endl;
		ss << "Unknown parameter names, ignoring them:";
		for(const auto &unknown_param: unknown_params_)
		{
			ss << std::endl;
			ss << " - '" << unknown_param << "'";
		}
		ss << std::endl;
	}
	if(!wrong_type_params_.empty())
	{
		ss << std::endl;
		ss << "Parameters set with *wrong types*, this can cause undefined behavior:";
		for(const auto &wrong_type_param: wrong_type_params_)
		{
			ss << std::endl;
			ss << " - '" << wrong_type_param << "'";
		}
		ss << std::endl;
	}
	if(!unknown_enum_.empty())
	{
		ss << std::endl;
		ss << "Unknown option in parameters, using default parameter option:";
		for(const auto &unknown_enum: unknown_enum_)
		{
			ss << std::endl;
			ss << " - '" << unknown_enum.second << "' in parameter: '" << unknown_enum.first << "'";
		}
		ss << std::endl;
	}
	ss << std::endl;
	ss << "Correct parameters and valid options for reference:" << std::endl;
	ss << T::printMeta(excluded_params);
	return ss.str();
}

} //namespace yafaray

#endif //LIBYAFARAY_PARAM_RESULT_H
