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
#include "common/param.h"
#include "common/logger.h"

BEGIN_YAFARAY

Light * Light::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Light");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	Light *light = nullptr;
	if(type == "arealight") light = AreaLight::factory(logger, scene, name, params);
	else if(type == "bgPortalLight") light = BackgroundPortalLight::factory(logger, scene, name, params);
	else if(type == "meshlight" || type == "objectlight") light = ObjectLight::factory(logger, scene, name, params);
	else if(type == "bglight") light = BackgroundLight::factory(logger, scene, name, params);
	else if(type == "directional") light = DirectionalLight::factory(logger, scene, name, params);
	else if(type == "ieslight") light = IesLight::factory(logger, scene, name, params);
	else if(type == "pointlight") light = PointLight::factory(logger, scene, name, params);
	else if(type == "spherelight") light = SphereLight::factory(logger, scene, name, params);
	else if(type == "spotlight") light = SpotLight::factory(logger, scene, name, params);
	else if(type == "sunlight") light = SunLight::factory(logger, scene, name, params);
	if(light) light->setName(name);
	return light;
}

END_YAFARAY
