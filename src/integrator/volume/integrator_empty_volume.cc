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

#include "integrator/volume/integrator_empty_volume.h"
#include "background/background.h"
#include "light/light.h"
#include "photon/photon.h"

BEGIN_YAFARAY

Rgba EmptyVolumeIntegrator::transmittance(RenderData &render_data, const Ray &ray) const {
	return Rgb(1.f);
}

Rgba EmptyVolumeIntegrator::integrate(RenderData &render_data, const Ray &ray, int additional_depth) const {
	return Rgba(0.f);
}

std::unique_ptr<Integrator> EmptyVolumeIntegrator::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	return std::unique_ptr<Integrator>(new EmptyVolumeIntegrator(logger));
}

END_YAFARAY
