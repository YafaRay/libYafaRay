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
#include "scene/scene_items.h"
#include "render/renderer.h"
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
class RenderView;
class Accelerator;
class FastRandom;

class VolumeIntegrator
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "VolumeIntegrator"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<VolumeIntegrator>, ParamResult> factory(Logger &logger, const yafaray::SceneItems<VolumeRegion> &volume_regions, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		virtual ~VolumeIntegrator() = default;
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		virtual bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene, const Renderer &renderer) = 0;
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
			PARAM_INIT;
		} params_;
		explicit VolumeIntegrator(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : params_{param_result, param_map}, logger_{logger} { }

		Logger &logger_;
};

} //namespace yafaray

#endif //LIBYAFARAY_INTEGRATOR_VOLUME_H
