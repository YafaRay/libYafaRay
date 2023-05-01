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
#include "integrator/surface/integrator_surface.h"

yafaray_SurfaceIntegrator *yafaray_createSurfaceIntegrator(yafaray_Logger *logger, const char *name, const yafaray_ParamMap *param_map)
{
	if(!logger || !name || !param_map) return nullptr;
	auto [surface_integrator, result]{yafaray::SurfaceIntegrator::factory(*reinterpret_cast<yafaray::Logger *>(logger), name, *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return reinterpret_cast<yafaray_SurfaceIntegrator *>(surface_integrator.release());
}

void yafaray_destroySurfaceIntegrator(yafaray_SurfaceIntegrator *surface_integrator)
{
	delete reinterpret_cast<yafaray::SurfaceIntegrator *>(surface_integrator);
}

yafaray_ResultFlags yafaray_defineVolumeIntegrator(yafaray_SurfaceIntegrator *surface_integrator, const yafaray_Scene *scene, const yafaray_ParamMap *param_map)
{
	if(!surface_integrator || !scene || !param_map) return YAFARAY_RESULT_ERROR_WHILE_CREATING;
	auto creation_result{reinterpret_cast<yafaray::SurfaceIntegrator *>(surface_integrator)->defineVolumeIntegrator(*reinterpret_cast<const yafaray::Scene *>(scene), *reinterpret_cast<const yafaray::ParamMap *>(param_map))};
	return static_cast<yafaray_ResultFlags>(creation_result.flags_.value());
}

void yafaray_preprocessSurfaceIntegrator(yafaray_RenderMonitor *render_monitor, yafaray_SurfaceIntegrator *surface_integrator, const yafaray_RenderControl *render_control, const yafaray_Scene *scene)
{
	if(!render_control || !render_monitor || !surface_integrator || !scene) return;
	reinterpret_cast<yafaray::SurfaceIntegrator *>(surface_integrator)->preprocess(*reinterpret_cast<yafaray::RenderMonitor *>(render_monitor), *reinterpret_cast<const yafaray::RenderControl *>(render_control), *reinterpret_cast<const yafaray::Scene *>(scene));
}

void yafaray_render(yafaray_RenderControl *render_control, yafaray_RenderMonitor *render_monitor, yafaray_SurfaceIntegrator *surface_integrator, yafaray_Film *film)
{
	if(!render_control || !render_monitor || !surface_integrator || !film) return;
	reinterpret_cast<yafaray::SurfaceIntegrator *>(surface_integrator)->render(*reinterpret_cast<yafaray::RenderControl *>(render_control), *reinterpret_cast<yafaray::RenderMonitor *>(render_monitor), reinterpret_cast<yafaray::ImageFilm *>(film));
}
