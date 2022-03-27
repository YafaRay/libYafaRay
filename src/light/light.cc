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

Light * Light::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Light");
		params.logContents(logger);
	}
	std::string type;
	std::string name;
	params.getParam("type", type);
	params.getParam("name", name);
	Light *light = nullptr;
	if(type == "arealight") light = AreaLight::factory(logger, params, scene);
	else if(type == "bgPortalLight") light = BackgroundPortalLight::factory(logger, params, scene);
	else if(type == "meshlight" || type == "objectlight") light = ObjectLight::factory(logger, params, scene);
	else if(type == "bglight") light = BackgroundLight::factory(logger, params, scene);
	else if(type == "directional") light = DirectionalLight::factory(logger, params, scene);
	else if(type == "ieslight") light = IesLight::factory(logger, params, scene);
	else if(type == "pointlight") light = PointLight::factory(logger, params, scene);
	else if(type == "spherelight") light = SphereLight::factory(logger, params, scene);
	else if(type == "spotlight") light = SpotLight::factory(logger, params, scene);
	else if(type == "sunlight") light = SunLight::factory(logger, params, scene);
	
	light->setName(name);
	return light;
}

END_YAFARAY
