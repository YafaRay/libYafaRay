#include <yafray_config.h>
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
#include <cmath>

__BEGIN_YAFRAY

// for removing all participating media effects

class YAFRAYPLUGIN_EXPORT EmptyVolumeIntegrator : public volumeIntegrator_t {
	public:
	EmptyVolumeIntegrator() {}

	virtual colorA_t transmittance(renderState_t &state, ray_t &ray) const {
		return color_t(1.f);
	}
	
	virtual colorA_t integrate(renderState_t &state, ray_t &ray) const {
		return colorA_t(0.f);
	}
	
	static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render)
	{
		return new EmptyVolumeIntegrator();
	}

};

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("none", EmptyVolumeIntegrator::factory);
	}

}

__END_YAFRAY
