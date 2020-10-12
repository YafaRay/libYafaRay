/****************************************************************************
 *
 *      This is part of the libYafaRay package
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
 *
 */

#include "render/render_view.h"
#include "common/param.h"
#include "scene/scene.h"
#include "common/string.h"
#include "common/logger.h"
#include "light/light.h"

BEGIN_YAFARAY

RenderView *RenderView::factory(ParamMap &params, const Scene &scene)
{
	Y_DEBUG PRTEXT(**RenderView) PREND; params.printDebug();
	std::string name;
	std::string camera_name;
	std::string light_names; //Separated by semicolon ";"
	float wavelength = 0.f;
	params.getParam("name", name);
	params.getParam("camera_name", camera_name);
	params.getParam("light_names", light_names);
	params.getParam("wavelength", wavelength);

	const Camera *camera = scene.getCamera(camera_name);
	const std::map<std::string, Light *> &scene_lights = scene.getLights();

	std::map<std::string, Light *> selected_lights;
	const std::vector<std::string> selected_lights_names = tokenize__(light_names, ";");

	if(selected_lights_names.empty()) selected_lights = scene_lights;
	else
	{
		for(const auto &light_name : selected_lights_names)
		{
			const Light *light = scene.getLight(light_name);
			if(!light)
			{
				Y_WARNING << "RenderView: view '" << name << "' could not find light '" << light_name << "', skipping..." << YENDL;
				continue;
			}
			selected_lights[light_name] = scene.getLight(light_name);
		}
	}
	return new RenderView(name, camera, selected_lights, wavelength);
}

const std::vector<Light *> RenderView::getLightsVisible() const
{
	std::vector<Light *> result;
	for(const auto &l : lights_)
	{
		if(l.second->lightEnabled() && !l.second->photonOnly()) result.push_back(l.second);
	}
	return result;
}

const std::vector<Light *> RenderView::getLightsEmittingCausticPhotons() const
{
	std::vector<Light *> result;
	for(const auto &l : lights_)
	{
		if(l.second->lightEnabled() && l.second->shootsCausticP()) result.push_back(l.second);
	}
	return result;
}

const std::vector<Light *> RenderView::getLightsEmittingDiffusePhotons() const
{
	std::vector<Light *> result;
	for(const auto &l : lights_)
	{
		if(l.second->lightEnabled() && l.second->shootsDiffuseP()) result.push_back(l.second);
	}
	return result;
}

END_YAFARAY