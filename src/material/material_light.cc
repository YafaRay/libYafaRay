/****************************************************************************
 *      material_light.cc: a material intended for visible light sources, i.e.
 *      it has no other properties than emiting light in conformance to uniform
 *      surface light sources (area, sphere, mesh lights...)
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

#include "material/material_light.h"
#include "geometry/surface.h"
#include "scene/scene.h"
#include "param/param.h"
#include "photon/photon.h"
#include "material/sample.h"
#include "scene/scene.h"

namespace yafaray {

LightMaterial::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(color_);
	PARAM_LOAD(power_);
	PARAM_LOAD(double_sided_);
}

ParamMap LightMaterial::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(color_);
	PARAM_SAVE(power_);
	PARAM_SAVE(double_sided_);
	PARAM_SAVE_END;
}

ParamMap LightMaterial::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Material>, ParamError> LightMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto material{std::make_unique<ThisClassType_t>(logger, param_error, param_map, scene.getMaterials())};
	if(param_error.notOk()) logger.logWarning(param_error.print<LightMaterial>(name, {"type"}));
	return {std::move(material), param_error};
}

LightMaterial::LightMaterial(Logger &logger, ParamError &param_error, const ParamMap &param_map, const SceneItems <Material> &materials) :
		ParentClassType_t{logger, param_error, param_map, materials}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	bsdf_flags_ = BsdfFlags::Emit;
}

Rgb LightMaterial::sample(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	s.pdf_ = 0.f;
	w = 0.f;
	return Rgb{0.f};
}

Rgb LightMaterial::emit(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo) const
{
	if(params_.double_sided_) return light_col_;
	const float angle = wo * sp.n_;
	return angle > 0 ? light_col_ : Rgb(0.f);
}

float LightMaterial::pdf(const MaterialData *mat_data, const SurfacePoint &sp, const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const
{
	return 0.f;
}

} //namespace yafaray
