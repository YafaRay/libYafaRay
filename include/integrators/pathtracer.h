#pragma once

#ifndef YAFARAY_PATHTRACER_H
#define YAFARAY_PATHTRACER_H

#include <yafray_constants.h>

#include <yafraycore/timer.h>
#include <yafraycore/photon.h>
#include <yafraycore/spectrum.h>
#include <yafraycore/scr_halton.h>

#include <core_api/mcintegrator.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <core_api/volume.h>

#include <utilities/mcqmc.h>

#include <sstream>
#include <iomanip>

BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT PathIntegrator: public MonteCarloIntegrator
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

END_YAFRAY

#endif // PATHTRACER
