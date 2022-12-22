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
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

VolumeIntegrator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
}

ParamMap VolumeIntegrator::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap VolumeIntegrator::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<std::unique_ptr<VolumeIntegrator>, ParamResult> VolumeIntegrator::factory(Logger &logger, const Items <VolumeRegion> &volume_regions, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Emission: return EmissionIntegrator::factory(logger, param_map, volume_regions);
		case Type::SingleScatter: return SingleScatterIntegrator::factory(logger, param_map, volume_regions);
		case Type::Sky: return SkyIntegrator::factory(logger, param_map, volume_regions);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

Rgb VolumeIntegrator::integrate(RandomGenerator &random_generator, const Ray &ray) const
{
	return integrate(random_generator, ray, 0);
}

} //namespace yafaray
