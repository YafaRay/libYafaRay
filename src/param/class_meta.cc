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
 */

#include "param/class_meta.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

ParamError ClassMeta::check(const ParamMap &param_map, const std::vector<std::string> &excluded_params, const std::vector<std::string> &excluded_params_starting_with) const
{
	ParamError param_error;
	for(const auto &[param_name, param] : param_map)
	{
		bool skip_param = false;
		for(const auto &excluded_param : excluded_params)
		{
			if(param_name == excluded_param)
			{
				skip_param = true;
				break;
			}
		}
		for(const auto &excluded_param_starting_with : excluded_params_starting_with)
		{
			if(param_name.rfind(excluded_param_starting_with, 0) == 0)
			{
				skip_param = true;
				break;
			}
		}
		if(skip_param) continue;
		const auto param_meta{find(param_name)};
		if(!param_meta)
		{
			param_error.flags_ |= ResultFlags{ResultFlags::WarningUnknownParam};
			param_error.unknown_params_.emplace_back(param_name);
			continue;
		}
		else if(param_meta->isEnum())
		{
			std::string param_value_str;
			param.getVal(param_value_str);
			if(!param_meta->enumContains(param_value_str))
			{
				param_error.flags_ |= ResultFlags{ResultFlags::WarningUnknownEnumOption};
				param_error.unknown_enum_.emplace_back(std::pair<std::string, std::string>{param_name, param_value_str});
				continue;
			}
		}
	}
	return param_error;
}

} //namespace yafaray
