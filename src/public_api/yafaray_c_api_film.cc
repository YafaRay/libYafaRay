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
#include "render/imagefilm.h"

yafaray_Film *yafaray_createFilm(yafaray_Logger *logger, yafaray_SurfaceIntegrator *surface_integrator, const char *name, const yafaray_ParamMap *param_map)
{
	if(!logger || !surface_integrator || !name || !param_map) return nullptr;
	auto [image_film, image_film_result]{yafaray::ImageFilm::factory(*reinterpret_cast<yafaray::Logger *>(logger), name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return reinterpret_cast<yafaray_Film *>(image_film.release());
}

void yafaray_destroyFilm(yafaray_Film *film)
{
	delete reinterpret_cast<yafaray::ImageFilm *>(film);
}

char *yafaray_getFilmName(yafaray_Film *film)
{
	return createCharString(reinterpret_cast<const yafaray::ImageFilm *>(film)->getName());
}

yafaray_ResultFlags yafaray_defineCamera(yafaray_Film *film, const yafaray_ParamMap *param_map)
{
	if(!film) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::ImageFilm *>(film)->defineCamera(*reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

yafaray_ResultFlags yafaray_createOutput(yafaray_Film *film, const char *name, const yafaray_ParamMap *param_map)
{
	if(!film || !name || !param_map) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::ImageFilm *>(film)->createOutput(name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.second.flags_.value());
}

void yafaray_setNotifyLayerCallback(yafaray_Film *film, yafaray_FilmNotifyLayerCallback callback, void *callback_data)
{
	if(!film) return;
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderNotifyLayerCallback(callback, callback_data);
}

void yafaray_setPutPixelCallback(yafaray_Film *film, yafaray_FilmPutPixelCallback callback, void *callback_data)
{
	if(!film) return;
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderPutPixelCallback(callback, callback_data);
}

void yafaray_setHighlightPixelCallback(yafaray_Film *film, yafaray_FilmHighlightPixelCallback callback, void *callback_data)
{
	if(!film) return;
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderHighlightPixelCallback(callback, callback_data);
}

void yafaray_setFlushAreaCallback(yafaray_Film *film, yafaray_FilmFlushAreaCallback callback, void *callback_data)
{
	if(!film) return;
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderFlushAreaCallback(callback, callback_data);
}

void yafaray_setFlushCallback(yafaray_Film *film, yafaray_FilmFlushCallback callback, void *callback_data)
{
	if(!film) return;
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderFlushCallback(callback, callback_data);
}

void yafaray_setHighlightAreaCallback(yafaray_Film *film, yafaray_FilmHighlightAreaCallback callback, void *callback_data)
{
	if(!film) return;
	reinterpret_cast<yafaray::ImageFilm *>(film)->setRenderHighlightAreaCallback(callback, callback_data);
}

int yafaray_getFilmWidth(const yafaray_Film *film)
{
	if(!film) return 0;
	return reinterpret_cast<const yafaray::ImageFilm *>(film)->getWidth();
}

int yafaray_getFilmHeight(const yafaray_Film *film)
{
	if(!film) return 0;
	return reinterpret_cast<const yafaray::ImageFilm *>(film)->getHeight();
}

void yafaray_defineLayer(yafaray_Film *film, const yafaray_ParamMap *param_map)
{
	if(!film || !param_map) return;
	reinterpret_cast<yafaray::ImageFilm *>(film)->defineLayer(*reinterpret_cast<const yafaray::ParamMap *>(param_map));
}

char *yafaray_getLayersTable(const yafaray_Film *film)
{
	if(!film) return nullptr;
	return createCharString(reinterpret_cast<const yafaray::ImageFilm *>(film)->getLayers()->printExportedTable());
}
