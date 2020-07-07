#pragma once

#ifndef Y_DIRECTLIGHT_H
#define Y_DIRECTLIGHT_H

#include <yafray_constants.h>

#include <yafraycore/timer.h>

#include <core_api/mcintegrator.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/background.h>
#include <core_api/light.h>

#include <sstream>
#include <iomanip>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT directLighting_t: public mcIntegrator_t
{
        public:
                directLighting_t(bool transpShad=false, int shadowDepth=4, int rayDepth=6);
                virtual bool preprocess();
                virtual colorA_t integrate(renderState_t &state, diffRay_t &ray, colorPasses_t &colorPasses, int additionalDepth = 0) const;
                static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
};

__END_YAFRAY

#endif // DIRECTLIGHT
