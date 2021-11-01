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

#include "volume/volumehandler_beer.h"
#include "scene/scene.h"
#include "material/material.h"
#include "common/param.h"

BEGIN_YAFARAY

BeerVolumeHandler::BeerVolumeHandler(Logger &logger, const Rgb &acol, double dist) : VolumeHandler(logger)
{
	const float maxlog = math::log(1e38f);
	sigma_a_.r_ = (acol.r_ > 1e-38f) ? -log(acol.r_) : maxlog;
	sigma_a_.g_ = (acol.g_ > 1e-38f) ? -log(acol.g_) : maxlog;
	sigma_a_.b_ = (acol.b_ > 1e-38f) ? -log(acol.b_) : maxlog;
	if(dist != 0.f) sigma_a_ *= 1.f / dist;
}

Rgb BeerVolumeHandler::transmittance(const Ray &ray) const
{
	if(ray.tmax_ < 0.f || ray.tmax_ > 1e30f) return {0.f}; //infinity check...
	const float dist = ray.tmax_; // maybe substract ray.tmin...
	const Rgb be(-dist * sigma_a_);
	const Rgb col = Rgb(math::exp(be.getR()), math::exp(be.getG()), math::exp(be.getB()));
	return col;
}

bool BeerVolumeHandler::scatter(const Ray &ray, Ray &s_ray, PSample &s) const
{
	return false;
}

std::unique_ptr<VolumeHandler> BeerVolumeHandler::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	Rgb a_col(0.5f);
	double dist = 1.f;
	params.getParam("absorption_col", a_col);
	params.getParam("absorption_dist", dist);
	return std::unique_ptr<VolumeHandler>(new BeerVolumeHandler(logger, a_col, dist));
}

END_YAFARAY
