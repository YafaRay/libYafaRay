
#ifndef Y_PATHTRACER_H
#define Y_PATHTRACER_H

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

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT pathIntegrator_t: public mcIntegrator_t
{
        public:
                pathIntegrator_t(bool transpShad=false, int shadowDepth=4);
                virtual bool preprocess();
                virtual colorA_t integrate(renderState_t &state, diffRay_t &ray, colorPasses_t &colorPasses, int additionalDepth = 0) const;
                static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
                enum { NONE, PATH, PHOTON, BOTH };
        protected:
                bool traceCaustics; //!< use path tracing for caustics (determined by causticType)
                bool no_recursive;
                float invNPaths;
                int causticType;
                int russianRouletteMinBounces;  //!< minimum number of bounces where russian roulette is not applied. Afterwards russian roulette will be used until the maximum selected bounces. If min_bounces >= max_bounces, then no russian roulette takes place
};

__END_YAFRAY

#endif // PATHTRACER
