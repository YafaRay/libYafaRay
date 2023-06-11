/****************************************************************************
 *   This is part of the libYafaRay package
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "public_api/yafaray_c_api.h"
#include "public_api/yafaray_c_api_utils.h"
#include "common/container.h"

yafaray_Container *yafaray_createContainer()
{
	return reinterpret_cast<yafaray_Container *>(new yafaray::Container());
}

void yafaray_destroyContainerButNotContainedPointers(yafaray_Container *container)
{
	delete reinterpret_cast<yafaray::Container *>(container);
}

void yafaray_destroyContainerAndContainedPointers(yafaray_Container *container)
{
	auto yaf_container{reinterpret_cast<yafaray::Container *>(container)};
	yaf_container->destroyContainedPointers();
	delete yaf_container;
}

void yafaray_addSceneToContainer(yafaray_Container *container, yafaray_Scene *scene)
{
	if(!container || !scene) return;
	reinterpret_cast<yafaray::Container *>(container)->addScene(reinterpret_cast<yafaray::Scene *>(scene));
}

void yafaray_addSurfaceIntegratorToContainer(yafaray_Container *container, yafaray_SurfaceIntegrator *surface_integrator)
{
	if(!container || !surface_integrator) return;
	reinterpret_cast<yafaray::Container *>(container)->addSurfaceIntegrator(reinterpret_cast<yafaray::SurfaceIntegrator *>(surface_integrator));
}

void yafaray_addFilmToContainer(yafaray_Container *container, yafaray_Film *film)
{
	if(!container || !film) return;
	reinterpret_cast<yafaray::Container *>(container)->addImageFilm(reinterpret_cast<yafaray::ImageFilm *>(film));
}

size_t yafaray_numberOfScenesInContainer(const yafaray_Container *container)
{
	if(!container) return 0;
	return reinterpret_cast<const yafaray::Container *>(container)->numScenes();
}

size_t yafaray_numberOfSurfaceIntegratorsInContainer(const yafaray_Container *container)
{
	if(!container) return 0;
	return reinterpret_cast<const yafaray::Container *>(container)->numSurfaceIntegrators();
}

size_t yafaray_numberOfFilmsInContainer(const yafaray_Container *container)
{
	if(!container) return 0;
	return reinterpret_cast<const yafaray::Container *>(container)->numImageFilms();
}

yafaray_Scene *yafaray_getSceneFromContainerByIndex(const yafaray_Container *container, size_t index)
{
	if(!container) return NULL;
	return reinterpret_cast<yafaray_Scene *>(reinterpret_cast<const yafaray::Container *>(container)->getScene(index));
}

yafaray_Scene *yafaray_getSceneFromContainerByName(const yafaray_Container *container, const char *name)
{
	if(!container || !name) return NULL;
	return reinterpret_cast<yafaray_Scene *>(reinterpret_cast<const yafaray::Container *>(container)->getScene(name));
}

yafaray_SurfaceIntegrator *yafaray_getSurfaceIntegratorFromContainerByIndex(const yafaray_Container *container, size_t index)
{
	if(!container) return NULL;
	return reinterpret_cast<yafaray_SurfaceIntegrator *>(reinterpret_cast<const yafaray::Container *>(container)->getSurfaceIntegrator(index));
}

yafaray_SurfaceIntegrator *yafaray_getSurfaceIntegratorFromContainerByName(const yafaray_Container *container, const char *name)
{
	if(!container || !name) return NULL;
	return reinterpret_cast<yafaray_SurfaceIntegrator *>(reinterpret_cast<const yafaray::Container *>(container)->getSurfaceIntegrator(name));
}

yafaray_Film *yafaray_getFilmFromContainerByIndex(const yafaray_Container *container, size_t index)
{
	if(!container) return NULL;
	return reinterpret_cast<yafaray_Film *>(reinterpret_cast<const yafaray::Container *>(container)->getImageFilm(index));
}

yafaray_Film *yafaray_getFilmFromContainerByName(const yafaray_Container *container, const char *name)
{
	if(!container || !name) return NULL;
	return reinterpret_cast<yafaray_Film *>(reinterpret_cast<const yafaray::Container *>(container)->getImageFilm(name));
}

char *yafaray_exportContainerToString(const yafaray_Container *container, yafaray_ContainerExportType container_export_type, yafaray_Bool only_export_non_default_parameters)
{
	if(!container) return NULL;
	return createCharString(reinterpret_cast<const yafaray::Container *>(container)->exportToString(container_export_type, static_cast<bool>(only_export_non_default_parameters)));
}

yafaray_ResultFlags yafaray_exportContainerToFile(const yafaray_Container *container, yafaray_ContainerExportType container_export_type, yafaray_Bool only_export_non_default_parameters, const char *file_path)
{
	if(!container) return YAFARAY_RESULT_ERROR_NOT_FOUND;
	return reinterpret_cast<const yafaray::Container *>(container)->exportToFile(container_export_type, static_cast<bool>(only_export_non_default_parameters), file_path);
}