/****************************************************************************
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
 */

#include "common/container.h"
#include "scene/scene.h"
#include "render/imagefilm.h"
#include "integrator/surface/integrator_surface.h"
#include <sstream>

namespace yafaray {

Scene *Container::getScene(const std::string &name) const
{
	for(auto scene : scenes_)
	{
		if(scene->getName() == name) return scene;
	}
	return nullptr;
}

SurfaceIntegrator *Container::getSurfaceIntegrator(const std::string &name) const
{
	for(auto surface_integrator : surface_integrators_)
	{
		if(surface_integrator->getName() == name) return surface_integrator;
	}
	return nullptr;
}

ImageFilm *Container::getImageFilm(const std::string &name) const
{
	for(auto image_film : image_films_)
	{
		if(image_film->getName() == name) return image_film;
	}
	return nullptr;
}

std::string Container::exportToString(yafaray_ContainerExportType container_export_type, bool export_default_param_values) const
{
	std::stringstream ss;
	ss << "Scenes:" << std::endl;
	for(auto scene : scenes_)
	{
		ss << scene->getName() << std::endl;
	}
	ss << "Surface Integrators:" << std::endl;
	for(auto surface_integrator : surface_integrators_)
	{
		ss << surface_integrator->getName() << std::endl;
	}
	ss << "ImageFilms:" << std::endl;
	for(auto image_film : image_films_)
	{
		ss << image_film->getName() << std::endl;
	}
	return ss.str();
}
void Container::destroyContainedPointers()
{
	for(auto surface_integrator : surface_integrators_)
	{
		delete surface_integrator;
	}
	for(auto image_film : image_films_)
	{
		delete image_film;
	}
	for(auto scene : scenes_)
	{
		delete scene;
	}
}

} //namespace yafaray