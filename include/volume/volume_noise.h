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

#ifndef YAFARAY_VOLUME_NOISE_H
#define YAFARAY_VOLUME_NOISE_H

#include "volume/volume.h"

namespace yafaray {

struct PSample;
class ParamMap;
class Scene;
class Texture;

class NoiseVolumeRegion final : public DensityVolumeRegion
{
	public:
		static VolumeRegion *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		NoiseVolumeRegion(Logger &logger, Rgb sa, Rgb ss, Rgb le, float gg, float cov, float sharp, float dens,
						  Point3 pmin, Point3 pmax, int attgrid_scale, Texture *noise) :
				DensityVolumeRegion(logger, sa, ss, le, gg, pmin, pmax, attgrid_scale)
		{
			tex_dist_noise_ = noise;
			cover_ = cov;
			sharpness_ = sharp * sharp;
			density_ = dens;
		}
		float density(const Point3 &p) const override;

		Texture *tex_dist_noise_;
		float cover_;
		float sharpness_;
		float density_;
};

} //namespace yafaray

#endif // YAFARAY_VOLUME_NOISE_H