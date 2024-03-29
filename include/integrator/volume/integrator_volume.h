#pragma once
/****************************************************************************
 *      integrator_volume.h: the interface definition for light volume integrators
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein (Lynx)
 *      Copyright (C) 2010  Rodrigo Placencia (DarkTide)
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

#ifndef LIBYAFARAY_INTEGRATOR_VOLUME_H
#define LIBYAFARAY_INTEGRATOR_VOLUME_H

#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include "common/items.h"
#include <map>
#include <memory>

namespace yafaray {

class Ray;
class RandomGenerator;
class Rgb;
class ParamMap;
class VolumeRegion;
class Scene;
class ImageFilm;
class Accelerator;
class FastRandom;
class SurfaceIntegrator;

class VolumeIntegrator
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "VolumeIntegrator"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<VolumeIntegrator>, ParamResult> factory(Logger &logger, const yafaray::Items<VolumeRegion> &volume_regions, const ParamMap &param_map);
		[[nodiscard]] virtual std::map<std::string, const ParamMeta *> getParamMetaMap() const = 0;
		[[nodiscard]] std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const;
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		virtual ~VolumeIntegrator() = default;
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		virtual bool preprocess(const Scene &scene, const SurfaceIntegrator &surf_integrator) = 0;
		Rgb integrate(RandomGenerator &random_generator, const Ray &ray) const;
		virtual Rgb transmittance(RandomGenerator &random_generator, const Ray &ray) const = 0;
		virtual Rgb integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const = 0;

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Emission, SingleScatter, Sky };
			inline static const EnumMap<ValueType_t> map_{{
					{"none", None, ""},
					{"EmissionIntegrator", Emission, ""},
					{"SingleScatterIntegrator", SingleScatter, ""},
					{"SkyIntegrator", Sky, ""},
				}};
		};
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
		} params_;
		explicit VolumeIntegrator(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : params_{param_result, param_map}, logger_{logger} { }

		Logger &logger_;
};

} //namespace yafaray

#endif //LIBYAFARAY_INTEGRATOR_VOLUME_H
