#pragma once

#ifndef YAFARAY_DIRECTLIGHT_H
#define YAFARAY_DIRECTLIGHT_H

#include <yafray_constants.h>

#include <yafraycore/timer.h>

#include <core_api/mcintegrator.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/background.h>
#include <core_api/light.h>

#include <sstream>
#include <iomanip>

BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT DirectLightIntegrator final : public MonteCarloIntegrator
{
	public:
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);

	private:
		DirectLightIntegrator(bool transp_shad = false, int shadow_depth = 4, int ray_depth = 6);
		virtual bool preprocess() override;
		virtual Rgba integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth = 0) const override;
};

END_YAFRAY

#endif // DIRECTLIGHT
