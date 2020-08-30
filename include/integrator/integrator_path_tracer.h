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

#include "constants.h"

#include "common/timer.h"
#include "common/photon.h"
#include "common/spectrum.h"
#include "common/scr_halton.h"

#include "integrator/integrator_montecarlo.h"
#include "common/environment.h"
#include "material/material.h"
#include "background/background.h"
#include "light/light.h"
#include "volume/volume.h"

#include "utility/util_mcqmc.h"

#include <sstream>
#include <iomanip>

BEGIN_YAFARAY

class PathIntegrator: public MonteCarloIntegrator
{
	public:
		PathIntegrator(bool transp_shad = false, int shadow_depth = 4);
		virtual bool preprocess();
		virtual Rgba integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth = 0) const;
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);
		enum { None, Path, Photon, Both };
	protected:
		bool trace_caustics_; //!< use path tracing for caustics (determined by causticType)
		bool no_recursive_;
		float inv_n_paths_;
		int caustic_type_;
		int russian_roulette_min_bounces_;  //!< minimum number of bounces where russian roulette is not applied. Afterwards russian roulette will be used until the maximum selected bounces. If min_bounces >= max_bounces, then no russian roulette takes place
};

END_YAFARAY

#endif // PATHTRACER
