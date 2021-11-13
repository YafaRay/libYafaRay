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

BEGIN_YAFARAY

class DirectLightIntegrator final : public MonteCarloIntegrator
{
	public:
		static std::unique_ptr<Integrator> factory(Logger &logger, ParamMap &params, const Scene &scene);

	private:
		DirectLightIntegrator(Logger &logger, bool transp_shad = false, int shadow_depth = 4, int ray_depth = 6);
		virtual std::string getShortName() const override { return "DL"; }
		virtual std::string getName() const override { return "DirectLight"; }
		virtual bool preprocess(const RenderControl &render_control, Timer &timer, const RenderView *render_view, ImageFilm *image_film) override;
		virtual Rgba integrate(int thread_id, int ray_level, bool chromatic_enabled, float wavelength, const Ray &ray, int additional_depth, const RayDivision &ray_division, ColorLayers *color_layers, const Camera *camera, RandomGenerator *random_generator, const PixelSamplingData &pixel_sampling_data, bool lights_geometry_material_emit) const override;
};

END_YAFARAY

#endif // DIRECTLIGHT
