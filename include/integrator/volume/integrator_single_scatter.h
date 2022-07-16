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

#ifndef YAFARAY_INTEGRATOR_SINGLE_SCATTER_H
#define YAFARAY_INTEGRATOR_SINGLE_SCATTER_H


#include "integrator/integrator.h"
#include <vector>
#include "render/render_view.h"

namespace yafaray {

class VolumeRegion;
class Light;
class Rgb;
class RandomGenerator;

class SingleScatterIntegrator final : public VolumeIntegrator
{
	public:
		static Integrator *factory(Logger &logger, RenderControl &render_control, const ParamMap &params, const Scene &scene);

	private:
		SingleScatterIntegrator(Logger &logger, float s_size, bool adapt, bool opt);
		std::string getShortName() const override { return "SSc"; }
		std::string getName() const override { return "SingleScatter"; }
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;
		// optical thickness, absorption, attenuation, extinction
		Rgb transmittance(RandomGenerator &random_generator, const Ray &ray) const override;
		// emission and in-scattering
		Rgb integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const override;
		Rgb getInScatter(RandomGenerator &random_generator, const Ray &step_ray, float current_step) const;

		bool adaptive_;
		bool optimize_;
		float adaptive_step_size_;
		std::vector<const Light *> lights_;
		unsigned int vr_size_;
		float i_vr_size_;
		float step_size_;
};

} //namespace yafaray

#endif // YAFARAY_INTEGRATOR_SINGLE_SCATTER_H