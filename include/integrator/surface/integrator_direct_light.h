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

#include "integrator_montecarlo.h"

namespace yafaray {

class DirectLightIntegrator final : public MonteCarloIntegrator
{
	public:
		static SurfaceIntegrator *factory(Logger &logger, RenderControl &render_control, const ParamMap &params, const Scene &scene);

	private:
		DirectLightIntegrator(RenderControl &render_control, Logger &logger, bool transparent_shadows = false, int shadow_depth = 4, int ray_depth = 6);
		std::string getShortName() const override { return "DL"; }
		std::string getName() const override { return "DirectLight"; }
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;
		std::pair<Rgb, float> integrate(Ray &ray, FastRandom &fast_random, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest) const override;
};

} //namespace yafaray

#endif // DIRECTLIGHT
