#pragma once
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

#ifndef LIBYAFARAY_CONTAINER_H
#define LIBYAFARAY_CONTAINER_H

#include "public_api/yafaray_c_api.h"
#include <vector>
#include <string>

namespace yafaray {

class Scene;
class SurfaceIntegrator;
class ImageFilm;

class Container final
{
	public:
		void addScene(Scene *scene) { scenes_.emplace_back(scene); }
		void addSurfaceIntegrator(SurfaceIntegrator *surface_integrator) { surface_integrators_.emplace_back(surface_integrator); }
		void addImageFilm(ImageFilm *image_film) { image_films_.emplace_back(image_film); }
		size_t numScenes() const { return scenes_.size(); }
		size_t numSurfaceIntegrators() const { return surface_integrators_.size(); }
		size_t numImageFilms() const { return image_films_.size(); }
		Scene *getScene(size_t index) const { if(index < scenes_.size()) return scenes_[index]; else return nullptr; }
		Scene *getScene(const std::string &name) const;
		SurfaceIntegrator *getSurfaceIntegrator(size_t index) const { if(index < scenes_.size()) return surface_integrators_[index]; else return nullptr; }
		SurfaceIntegrator *getSurfaceIntegrator(const std::string &name) const;
		ImageFilm *getImageFilm(size_t index) const { if(index < scenes_.size()) return image_films_[index]; else return nullptr; }
		ImageFilm *getImageFilm(const std::string &name) const;
		std::string exportToString(yafaray_ContainerExportType container_export_type, bool export_default_param_values) const;
		void destroyContainedPointers();

	private:
		std::vector<Scene *> scenes_;
		std::vector<SurfaceIntegrator *> surface_integrators_;
		std::vector<ImageFilm *> image_films_;
};

} //namespace yafaray

#endif //LIBYAFARAY_CONTAINER_H
