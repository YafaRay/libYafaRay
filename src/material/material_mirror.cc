/****************************************************************************
 *      material_mirror.cc: a simple mirror mat
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

#include "material/material_mirror.h"
#include "shader/shader_node.h"
#include "geometry/surface.h"
#include "common/logger.h"
#include "color/spectrum.h"
#include "param/param.h"
#include "material/sample.h"
#include "scene/scene.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> MirrorMaterial::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(color_);
	PARAM_META(reflect_);
	return param_meta_map;
}

MirrorMaterial::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(color_);
	PARAM_LOAD(reflect_);
}

ParamMap MirrorMaterial::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(color_);
	PARAM_SAVE(reflect_);
	return param_map;
}

std::pair<std::unique_ptr<Material>, ParamResult> MirrorMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto material{std::make_unique<MirrorMaterial>(logger, param_result, param_map, scene.getMaterials())};
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + material->getAsParamMap(true).print());
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(material), param_result};
}

MirrorMaterial::MirrorMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials) :
		ParentClassType_t{logger, param_result, param_map, materials}, params_{param_result, param_map}
{
	bsdf_flags_ = BsdfFlags::Specular;
}

Rgb MirrorMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	wi = Vec3f::reflectDir(sp.n_, wo);
	s.sampled_flags_ = BsdfFlags::Specular | BsdfFlags::Reflect;
	w = 1.f;
	return ref_col_ * (1.f / std::abs(sp.n_ * wi));
}

Specular MirrorMaterial::getSpecular(int ray_level, const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, bool chromatic, float wavelength) const
{
	Specular specular;
	specular.reflect_ = std::make_unique<DirectionColor>();
	specular.reflect_->col_ = ref_col_;
	const Vec3f n{SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo)};
	specular.reflect_->dir_ = Vec3f::reflectDir(n, wo);
	return specular;
}

} //namespace yafaray
