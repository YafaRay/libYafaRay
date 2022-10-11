/****************************************************************************
 *      material_null.cc: a "dummy" material, useful e.g. to keep photons from
 *      getting stored on surfaces that don't affect the scene
 *
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

#include "material/material_null.h"
#include "shader/shader_node.h"
#include "geometry/surface.h"
#include "common/logger.h"
#include "color/spectrum.h"
#include "param/param.h"
#include "material/sample.h"

namespace yafaray {

NullMaterial::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
}

ParamMap NullMaterial::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap NullMaterial::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Material::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Material *, ParamError> NullMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto mat = new NullMaterial(logger, param_error, param_map);
	if(param_error.notOk()) logger.logWarning(param_error.print<NullMaterial>(name, {"type"}));
	return {mat, param_error};
}

NullMaterial::NullMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		Material{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

Rgb NullMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	s.pdf_ = 0.f;
	w = 0.f;
	return Rgb{0.f};
}

} //namespace yafaray
