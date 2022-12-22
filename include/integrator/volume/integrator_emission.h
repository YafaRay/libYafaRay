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

#ifndef LIBYAFARAY_INTEGRATOR_EMISSION_H
#define LIBYAFARAY_INTEGRATOR_EMISSION_H

#include "integrator/volume/integrator_volume.h"

namespace yafaray {

template <typename T> class Items;

class EmissionIntegrator final : public VolumeIntegrator
{
		using ThisClassType_t = EmissionIntegrator; using ParentClassType_t = VolumeIntegrator;

	public:
		inline static std::string getClassName() { return "EmissionIntegrator"; }
		static std::pair<std::unique_ptr<VolumeIntegrator>, ParamResult> factory(Logger &logger, const ParamMap &params, const Items<VolumeRegion> &volume_regions);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		explicit EmissionIntegrator(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items<VolumeRegion> &volume_regions);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		[[nodiscard]] Type type() const override { return Type::Emission; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
		} params_;
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene, const Renderer &renderer) override { return true; }
		// optical thickness, absorption, attenuation, extinction
		Rgb transmittance(RandomGenerator &random_generator, const Ray &ray) const override;
		// emission part
		Rgb integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const override;

		const Items<VolumeRegion> &volume_regions_;
};

} //namespace yafaray

#endif // LIBYAFARAY_INTEGRATOR_EMISSION_H