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

#ifndef YAFARAY_INTEGRATOR_DIRECT_LIGHT_H
#define YAFARAY_INTEGRATOR_DIRECT_LIGHT_H

#include "integrator_photon_caustic.h"

namespace yafaray {

class DirectLightIntegrator final : public CausticPhotonIntegrator
{
	public:
		inline static std::string getClassName() { return "DirectLightIntegrator"; }
		static std::pair<SurfaceIntegrator *, ParamError> factory(Logger &logger, RenderControl &render_control, const ParamMap &params, const Scene &scene);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		[[nodiscard]] Type type() const override { return Type::DirectLight; }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		DirectLightIntegrator(RenderControl &render_control, Logger &logger, ParamError &param_error, const ParamMap &param_map);
		[[nodiscard]] std::string getName() const override { return "DirectLight"; }
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;
		std::pair<Rgb, float> integrate(Ray &ray, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest) const override;
};

} //namespace yafaray

#endif // DIRECTLIGHT
