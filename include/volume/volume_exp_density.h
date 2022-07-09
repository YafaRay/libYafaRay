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

#ifndef YAFARAY_VOLUME_EXP_DENSITY_H
#define YAFARAY_VOLUME_EXP_DENSITY_H

#include "volume/volume.h"

namespace yafaray {

struct PSample;
class ParamMap;
class Scene;

class ExpDensityVolumeRegion final : public DensityVolumeRegion
{
	public:
		static VolumeRegion *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		ExpDensityVolumeRegion(Logger &logger, const Rgb &sa, const Rgb &ss, const Rgb &le, float gg, const Point3f &pmin, const Point3f &pmax, int attgrid_scale, float aa, float bb);
		float density(const Point3f &p) const override;

		float a_, b_;
};

} //namespace yafaray

#endif // YAFARAY_VOLUME_EXP_DENSITY_H