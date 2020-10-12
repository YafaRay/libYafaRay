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

#include "integrator/integrator.h"

BEGIN_YAFARAY

class Background;

class SkyIntegrator : public VolumeIntegrator
{
	public:
		static Integrator *factory(ParamMap &params, const Scene &scene);

	private:
		SkyIntegrator(float s_size, float a, float ss, float t);
		virtual std::string getShortName() const override { return "Sky"; }
		virtual std::string getName() const override { return "Sky"; }
		virtual bool preprocess(const RenderControl &render_control, const RenderView *render_view) override;
		// optical thickness, absorption, attenuation, extinction
		virtual Rgba transmittance(RenderData &render_data, Ray &ray) const override;
		// emission and in-scattering
		virtual Rgba integrate(RenderData &render_data, Ray &ray, int additional_depth = 0) const override;
		Rgba skyTau(const Ray &ray) const;
		Rgba skyTau(const Ray &ray, float beta, float alpha) const;

		float step_size_;
		float alpha_; // steepness of the exponential density
		float sigma_t_; // beta in the paper, more or less the thickness coefficient
		float turbidity_;
		Background *background_;
		float b_m_, b_r_;
		float alpha_r_; // rayleigh, molecules
		float alpha_m_; // mie, haze
		float scale_;
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_SKY_H