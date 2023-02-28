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
 */

#ifndef LIBYAFARAY_VOLUME_HANDLER_H
#define LIBYAFARAY_VOLUME_HANDLER_H

#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include <memory>

namespace yafaray {

class Scene;
class Ray;
struct PSample;

class VolumeHandler
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "VolumeHandler"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<VolumeHandler>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		VolumeHandler(Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		virtual Rgb transmittance(const Ray &ray) const = 0;
		virtual bool scatter(const Ray &ray, Ray &s_ray, PSample &s) const = 0;
		virtual ~VolumeHandler() = default;

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Beer, Sss };
			inline static const EnumMap<ValueType_t> map_{{
					{"beer", Beer, ""},
					{"sss", Sss, ""},
				}};
		};
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
		} params_;
		Logger &logger_;
};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_HANDLER_H
