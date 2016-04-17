
#ifndef Y_PHOTONINTEGR_H
#define Y_PHOTONINTEGR_H

#include <yafray_config.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <core_api/imagefilm.h>
#include <core_api/camera.h>
#include <core_api/mcintegrator.h>
#include <yafraycore/photon.h>
#include <yafraycore/monitor.h>
#include <yafraycore/ccthreads.h>
#include <yafraycore/timer.h>
#include <yafraycore/spectrum.h>
#include <utilities/sample_utils.h>


__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT photonIntegrator_t: public mcIntegrator_t
{
	public:
		photonIntegrator_t(unsigned int dPhotons, unsigned int cPhotons, bool transpShad=false, int shadowDepth = 4, float dsRad = 0.1f, float cRad = 0.01f);
		~photonIntegrator_t();
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray, colorPasses_t &colorPasses, int additionalDepth = 0) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		color_t finalGathering(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, colorPasses_t &colorPasses) const;
		
		bool caustics_enabled() const { return bEnableCaustics; }
		bool diffuse_enabled() const { return bEnableDiffuse; }
		void enableCaustics(const bool caustics) { bEnableCaustics = caustics; }
		void enableDiffuse(const bool diffuse) { bEnableDiffuse = diffuse; }
		
		bool bEnableCaustics, bEnableDiffuse; //!< enable/disable caustics or diffuse photon processing
		bool finalGather, showMap;
		bool prepass;
		unsigned int nDiffusePhotons;
		int nDiffuseSearch;
		int gatherBounces;
		float dsRadius; //!< diffuse search radius
		float lookupRad; //!< square radius to lookup radiance photons, as infinity is no such good idea ;)
		float gatherDist; //!< minimum distance to terminate path tracing (unless gatherBounces is reached)
		friend class prepassWorker_t;
};

__END_YAFRAY

#endif // Y_PHOTONINTEGR_H
