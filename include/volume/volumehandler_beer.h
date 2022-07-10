#pragma once
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

#ifndef YAFARAY_VOLUMEHANDLER_BEER_H
#define YAFARAY_VOLUMEHANDLER_BEER_H

#include "volume/volume.h"

namespace yafaray {

class ParamMap;
class Scene;

class BeerVolumeHandler : public VolumeHandler
{
	public:
		static VolumeHandler *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	protected:
		BeerVolumeHandler(Logger &logger, const Rgb &sigma): VolumeHandler(logger), sigma_a_(sigma) {};
		BeerVolumeHandler(Logger &logger, const Rgb &acol, double dist);

	private:
		Rgb transmittance(const Ray<float> &ray) const override;
		bool scatter(const Ray<float> &ray, Ray<float> &s_ray, PSample &s) const override;
		Rgb getSubSurfaceColor(const MaterialData &mat_data) const { return sigma_a_; }
		Rgb sigma_a_;
};

} //namespace yafaray

#endif // YAFARAY_VOLUMEHANDLER_BEER_H