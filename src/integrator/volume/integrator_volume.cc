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

#include "integrator/volume/integrator_volume.h"
#include "integrator/volume/integrator_sky.h"
#include "integrator/volume/integrator_single_scatter.h"
#include "integrator/volume/integrator_emission.h"
#include "common/param.h"
#include "scene/scene.h"

namespace yafaray {

VolumeIntegrator * VolumeIntegrator::factory(Logger &logger, RenderControl &render_control, const Scene &scene, const std::string &name, const ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("**VolumeIntegrator");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	if(type == "EmissionIntegrator") return EmissionIntegrator::factory(logger, render_control, params, scene);
	else if(type == "SingleScatterIntegrator") return SingleScatterIntegrator::factory(logger, render_control, params, scene);
	else if(type == "SkyIntegrator") return SkyIntegrator::factory(logger, render_control, params, scene);
	else return nullptr;
}

bool VolumeIntegrator::preprocess(FastRandom &fast_random, ImageFilm *image_film, const RenderView *render_view, const Scene &scene)
{
	bool success = Integrator::preprocess(fast_random, image_film, render_view, scene);
	volume_regions_ = scene.getVolumeRegions();
	return success && static_cast<bool>(volume_regions_);
}

Rgb VolumeIntegrator::integrate(RandomGenerator &random_generator, const Ray &ray) const
{
	return integrate(random_generator, ray, 0);
}

} //namespace yafaray
