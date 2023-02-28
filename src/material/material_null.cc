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
#include "scene/scene.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> NullMaterial::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	return param_meta_map;
}

NullMaterial::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
}

ParamMap NullMaterial::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	return param_map;
}

std::pair<std::unique_ptr<Material>, ParamResult> NullMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto material{std::make_unique<NullMaterial>(logger, param_result, param_map, scene.getMaterials())};

	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + material->getAsParamMap(true).print());
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(material), param_result};
}

NullMaterial::NullMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials) :
		ParentClassType_t{logger, param_result, param_map, materials}, params_{param_result, param_map}
{
}

Rgb NullMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	s.pdf_ = 0.f;
	w = 0.f;
	return Rgb{0.f};
}

} //namespace yafaray
