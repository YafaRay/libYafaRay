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

#ifndef YAFARAY_INTEGRATOR_PATH_TRACER_H
#define YAFARAY_INTEGRATOR_PATH_TRACER_H

#include "integrator/integrator_montecarlo.h"

BEGIN_YAFARAY

class PathIntegrator final : public MonteCarloIntegrator
{
	public:
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);

	private:
		PathIntegrator(bool transp_shad = false, int shadow_depth = 4);
		virtual bool preprocess() override;
		virtual Rgba integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth = 0) const override;
		enum class CausticType { None, Path, Photon, Both };

		bool trace_caustics_; //!< use path tracing for caustics (determined by causticType)
		bool no_recursive_;
		float inv_n_paths_;
		CausticType caustic_type_;
		int russian_roulette_min_bounces_;  //!< minimum number of bounces where russian roulette is not applied. Afterwards russian roulette will be used until the maximum selected bounces. If min_bounces >= max_bounces, then no russian roulette takes place
};

END_YAFARAY

#endif // PATHTRACER
