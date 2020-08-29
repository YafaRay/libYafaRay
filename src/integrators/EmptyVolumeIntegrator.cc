#include <yafray_constants.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/integrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <integrators/integr_utils.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>
#include <vector>

BEGIN_YAFRAY

// for removing all participating media effects

class YAFRAYPLUGIN_EXPORT EmptyVolumeIntegrator : public VolumeIntegrator
{
	public:
		EmptyVolumeIntegrator() {}

		virtual Rgba transmittance(RenderState &state, Ray &ray) const
		{
			return Rgb(1.f);
		}

		virtual Rgba integrate(RenderState &state, Ray &ray, ColorPasses &color_passes, int additional_depth /*=0*/) const
		{
			return Rgba(0.f);
		}

		static Integrator *factory(ParamMap &params, RenderEnvironment &render)
		{
			return new EmptyVolumeIntegrator();
		}

};

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("none", EmptyVolumeIntegrator::factory);
	}

}

END_YAFRAY
