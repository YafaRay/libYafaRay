/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "integrator/integrator.h"
#include "integrator/surface/integrator_bidirectional.h"
#include "integrator/surface/integrator_direct_light.h"
#include "integrator/surface/integrator_path_tracer.h"
#include "integrator/surface/integrator_photon_mapping.h"
#include "integrator/surface/integrator_sppm.h"
#include "integrator/surface/integrator_debug.h"
#include "integrator/volume/integrator_empty_volume.h"
#include "integrator/volume/integrator_sky.h"
#include "integrator/volume/integrator_single_scatter.h"
#include "integrator/volume/integrator_emission.h"
#include "common/param.h"

BEGIN_YAFARAY

Integrator *Integrator::factory(ParamMap &params, const Scene &scene)
{
	Y_DEBUG PRTEXT(**Integrator) PREND; params.printDebug();
	std::string type;
	params.getParam("type", type);
	if(type == "bidirectional")
	{
		Y_WARNING << "The Bidirectional integrator is UNSTABLE at the moment and needs to be improved. It might give unexpected and perhaps even incorrect render results. Use at your own risk." << YENDL;
		return BidirectionalIntegrator::factory(params, scene);
	}
	else if(type == "DebugIntegrator") return DebugIntegrator::factory(params, scene);
	else if(type == "directlighting") return DirectLightIntegrator::factory(params, scene);
	else if(type == "pathtracing") return PathIntegrator::factory(params, scene);
	else if(type == "photonmapping") return PhotonIntegrator::factory(params, scene);
	else if(type == "SPPM") return SppmIntegrator::factory(params, scene);
	else if(type == "none") return EmptyVolumeIntegrator::factory(params, scene);
	else if(type == "EmissionIntegrator") return EmissionIntegrator::factory(params, scene);
	else if(type == "SingleScatterIntegrator") return SingleScatterIntegrator::factory(params, scene);
	else if(type == "SkyIntegrator") return SkyIntegrator::factory(params, scene);
	else return nullptr;
}

END_YAFARAY
