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

std::map<std::string, const ParamMeta *> LightMaterial::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	PARAM_META(color_);
	PARAM_META(power_);
	PARAM_META(double_sided_);
	return param_meta_map;
}

LightMaterial::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(color_);
	PARAM_LOAD(power_);
	PARAM_LOAD(double_sided_);
}

ParamMap LightMaterial::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	PARAM_SAVE(color_);
	PARAM_SAVE(power_);
	PARAM_SAVE(double_sided_);
	return param_map;
}

std::pair<std::unique_ptr<Material>, ParamResult> LightMaterial::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map, const std::list<ParamMap> &nodes_param_maps)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	auto material{std::make_unique<LightMaterial>(logger, param_result, param_map, scene.getMaterials())};
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + material->getAsParamMap(true).print());
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(material), param_result};
}

LightMaterial::LightMaterial(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Material> &materials) :
		ParentClassType_t{logger, param_result, param_map, materials}, params_{param_result, param_map}
{
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

std::string LightMaterial::exportToString(yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	ss << "\t\t<material name=\"" << getName() << "\">" << std::endl;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << param_map.exportMap(3, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << "\t\t</material>" << std::endl;
	return ss.str();
}

} //namespace yafaray
