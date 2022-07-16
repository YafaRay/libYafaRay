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

#ifndef YAFARAY_INTEGRATOR_SKY_H
#define YAFARAY_INTEGRATOR_SKY_H

#include <math/random.h>
#include "integrator/integrator.h"

namespace yafaray {

class Background;

class SkyIntegrator : public VolumeIntegrator
{
	public:
		static Integrator *factory(Logger &logger, RenderControl &render_control, const ParamMap &params, const Scene &scene);

	private:
		SkyIntegrator(Logger &logger, float s_size, float a, float ss, float t);
		std::string getShortName() const override { return "Sky"; }
		std::string getName() const override { return "Sky"; }
		bool preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene) override;
		// optical thickness, absorption, attenuation, extinction
		Rgb transmittance(RandomGenerator &random_generator, const Ray &ray) const override;
		// emission and in-scattering
		Rgb integrate(RandomGenerator &random_generator, const Ray &ray, int additional_depth) const override;
		Rgb skyTau(const Ray &ray) const;
		Rgb skyTau(const Ray &ray, float beta, float alpha) const;
		static float mieScatter(float theta);

		float step_size_;
		float alpha_; // steepness of the exponential density
		float sigma_t_; // beta in the paper, more or less the thickness coefficient
		float turbidity_;
		const Background *background_ = nullptr;
		float b_m_, b_r_;
		float alpha_r_; // rayleigh, molecules
		float alpha_m_; // mie, haze
		float scale_;
};

} //namespace yafaray

#endif // YAFARAY_INTEGRATOR_SKY_H