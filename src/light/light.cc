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

#include "light/light.h"
#include "light/light_sun.h"
#include "light/light_spot.h"
#include "light/light_sphere.h"
#include "light/light_point.h"
#include "light/light_area.h"
#include "light/light_background.h"
#include "light/light_background_portal.h"
#include "light/light_directional.h"
#include "light/light_ies.h"
#include "light/light_object_light.h"
#include "common/items.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

std::map<std::string, const ParamMeta *> Light::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(light_enabled_);
	PARAM_META(cast_shadows_);
	PARAM_META(shoot_caustic_);
	PARAM_META(shoot_diffuse_);
	PARAM_META(photon_only_);
	return param_meta_map;
}

Light::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(light_enabled_);
	PARAM_LOAD(cast_shadows_);
	PARAM_LOAD(shoot_caustic_);
	PARAM_LOAD(shoot_diffuse_);
	PARAM_LOAD(photon_only_);
}

ParamMap Light::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_SAVE(light_enabled_);
	PARAM_SAVE(cast_shadows_);
	PARAM_SAVE(shoot_caustic_);
	PARAM_SAVE(shoot_diffuse_);
	PARAM_SAVE(photon_only_);
	return param_map;
}

std::pair<std::unique_ptr<Light>, ParamResult> Light::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Area: return AreaLight::factory(logger, scene, name, param_map);
		case Type::BackgroundPortal: return BackgroundPortalLight::factory(logger, scene, name, param_map);
		case Type::Object: return ObjectLight::factory(logger, scene, name, param_map);
		case Type::Background: return BackgroundLight::factory(logger, scene, name, param_map);
		case Type::Directional: return DirectionalLight::factory(logger, scene, name, param_map);
		case Type::Ies: return IesLight::factory(logger, scene, name, param_map);
		case Type::Point: return PointLight::factory(logger, scene, name, param_map);
		case Type::Sphere: return SphereLight::factory(logger, scene, name, param_map);
		case Type::Spot: return SpotLight::factory(logger, scene, name, param_map);
		case Type::Sun: return SunLight::factory(logger, scene, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

std::string Light::getName() const
{
	return lights_.findNameFromId(id_).first;
}

} //namespace yafaray
