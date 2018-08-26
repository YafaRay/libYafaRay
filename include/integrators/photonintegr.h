
#ifndef Y_PHOTONINTEGR_H
#define Y_PHOTONINTEGR_H

#include <yafray_config.h>

#include <yafraycore/timer.h>
#include <yafraycore/photon.h>
#include <yafraycore/spectrum.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/monitor.h>

#include <core_api/mcintegrator.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <core_api/imagefilm.h>
#include <core_api/camera.h>

#include <utilities/mcqmc.h>
#include <utilities/sample_utils.h>

#include <sstream>
#include <iomanip>

__BEGIN_YAFRAY

struct preGatherData_t
{
	preGatherData_t(photonMap_t *dm): diffuseMap(dm), fetched(0) {}
	photonMap_t *diffuseMap;
	
	std::vector<radData_t> rad_points;
	std::vector<photon_t> radianceVec;
	progressBar_t *pbar;
	volatile int fetched;
	std::mutex mutx;
};

class YAFRAYPLUGIN_EXPORT photonIntegrator_t: public mcIntegrator_t
{
	public:
		photonIntegrator_t(unsigned int dPhotons, unsigned int cPhotons, bool transpShad=false, int shadowDepth = 4, float dsRad = 0.1f, float cRad = 0.01f);
		~photonIntegrator_t();
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray, colorPasses_t &colorPasses, int additionalDepth = 0) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
		virtual void preGatherWorker(preGatherData_t * gdata, float dsRad, int nSearch);
		virtual void causticWorker(photonMap_t * causticMap, int threadID, const scene_t *scene, unsigned int nCausPhotons, const pdf1D_t *lightPowerD, int numCLights, const std::string &integratorName, const std::vector<light_t *> &tmplights, int causDepth, progressBar_t *pb, int pbStep, unsigned int &totalPhotonsShot, int maxBounces);
		virtual void diffuseWorker(photonMap_t * diffuseMap, int threadID, const scene_t *scene, unsigned int nDiffusePhotons, const pdf1D_t *lightPowerD, int numDLights, const std::string &integratorName, const std::vector<light_t *> &tmplights, progressBar_t *pb, int pbStep, unsigned int &totalPhotonsShot, int maxBounces, bool finalGather, preGatherData_t &pgdat);
		virtual void photonMapKdTreeWorker(photonMap_t * photonMap);

	protected:
		color_t finalGathering(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, colorPasses_t &colorPasses) const;
		
		void enableCaustics(const bool caustics) { usePhotonCaustics = caustics; }
		void enableDiffuse(const bool diffuse) { usePhotonDiffuse = diffuse; }
		
		bool usePhotonDiffuse; //!< enable/disable diffuse photon processing
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
