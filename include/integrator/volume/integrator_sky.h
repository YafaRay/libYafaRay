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

BEGIN_YAFARAY

class Background;

class SkyIntegrator : public VolumeIntegrator
{
	public:
		static std::unique_ptr<Integrator> factory(Logger &logger, ParamMap &params, const Scene &scene);

	private:
		SkyIntegrator(Logger &logger, float s_size, float a, float ss, float t);
		virtual std::string getShortName() const override { return "Sky"; }
		virtual std::string getName() const override { return "Sky"; }
		virtual bool preprocess(const RenderControl &render_control, Timer &timer, const RenderView *render_view, ImageFilm *image_film) override;
		// optical thickness, absorption, attenuation, extinction
		virtual Rgba transmittance(RandomGenerator *random_generator, const Ray &ray) const override;
		// emission and in-scattering
		virtual Rgba integrate(RandomGenerator *random_generator, const Ray &ray, int additional_depth = 0) const override;
		Rgba skyTau(const Ray &ray) const;
		Rgba skyTau(const Ray &ray, float beta, float alpha) const;
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

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_SKY_H