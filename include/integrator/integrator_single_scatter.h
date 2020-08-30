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

BEGIN_YAFARAY

class VolumeRegion;
class Light;
class Rgb;

class SingleScatterIntegrator : public VolumeIntegrator
{
	private:
		bool adaptive_;
		bool optimize_;
		float adaptive_step_size_;
		std::vector<VolumeRegion *> list_vr_;
		std::vector<Light *> lights_;
		unsigned int vr_size_;
		float i_vr_size_;

	public:
		SingleScatterIntegrator(float s_size, bool adapt, bool opt);
		virtual bool preprocess();
		Rgb getInScatter(RenderState &state, Ray &step_ray, float current_step) const;
		// optical thickness, absorption, attenuation, extinction
		virtual Rgba transmittance(RenderState &state, Ray &ray) const;
		// emission and in-scattering
		virtual Rgba integrate(RenderState &state, Ray &ray, ColorPasses &color_passes, int additional_depth /*=0*/) const;
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);
		float step_size_;
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_SINGLE_SCATTER_H