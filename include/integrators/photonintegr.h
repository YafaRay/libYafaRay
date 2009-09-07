
#ifndef Y_PHOTONINTEGR_H
#define Y_PHOTONINTEGR_H

#include <yafray_config.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <core_api/imagefilm.h>
#include <yafraycore/tiledintegrator.h>
#include <yafraycore/photon.h>
#include <yafraycore/old_photonmap.h>
#include <yafraycore/monitor.h>
#include <yafraycore/ccthreads.h>
#include <yafraycore/timer.h>
#include <yafraycore/spectrum.h>
#include <yafraycore/irradiancecache.h>
#include <utilities/sample_utils.h>
#include <integrators/integr_utils.h>


#define OLD_PMAP 0

__BEGIN_YAFRAY

// from common.cc
//color_t estimateDirect(renderState_t &state, const surfacePoint_t &sp, const std::vector<light_t *> &lights, scene_t *scene, const vector3d_t &wo, bool trShad, int sDepth);

class YAFRAYPLUGIN_EXPORT photonIntegrator_t: public tiledIntegrator_t
{
	public:
		photonIntegrator_t(int photons, bool transpShad=false, int shadowDepth=4, float dsRad=0.1);
		~photonIntegrator_t();
		virtual bool render(imageFilm_t *image);
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray/*, sampler_t &sam*/) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		color_t finalGathering(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo) const;
		void sampleIrrad(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, irradSample_t &ir) const;
		color_t estimateOneDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo, const std::vector<light_t *>  &lights, int d1, int n)const;
		bool renderIrradPass();
		bool progressiveTile(renderArea_t &a, int log_spacing, bool first, std::vector<irradSample_t> &samples, int threadID) const;
		bool progressiveTile2(renderArea_t &a, int log_spacing, bool first, std::vector<irradSample_t> &samples, int threadID) const;
		colorA_t fillIrradCache(renderState_t &state, PFLOAT x, PFLOAT y, bool first, std::vector<irradSample_t> &samples) const;
		colorA_t recFillCache(renderState_t &state, diffRay_t &c_ray, bool first, std::vector<irradSample_t> &samples) const;
		
		background_t *background;
		bool trShad;
		bool finalGather, showMap;
		bool cacheIrrad;
		bool use_bg;
		bool prepass;
		unsigned int nPhotons;
		int sDepth, rDepth, maxBounces, nSearch, nCausSearch;
		int nPaths, gatherBounces;
		PFLOAT dsRadius; //!< diffuse search radius
		PFLOAT lookupRad; //!< square radius to lookup radiance photons, as infinity is no such good idea ;)
		PFLOAT gatherDist; //!< minimum distance to terminate path tracing (unless gatherBounces is reached)
#if OLD_PMAP > 0
		globalPhotonMap_t diffuseMap, causticMap;
		globalPhotonMap_t radianceMap; //!< this map contains precomputed radiance "photons", not incoming photon marks
#else
		photonMap_t diffuseMap, causticMap;
		photonMap_t radianceMap; //!< this map contains precomputed radiance "photons", not incoming photon marks
#endif
		pdf1D_t *lightPowerD;
		std::vector<light_t*> lights;
		/* mutable  */irradianceCache_t irCache;
		friend class prepassWorker_t;
};

__END_YAFRAY

#endif // Y_PHOTONINTEGR_H
