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

#ifndef YAFARAY_VOLUME_GRID_H
#define YAFARAY_VOLUME_GRID_H

#include "volume/volume.h"

#include <fstream>
#include <cstdlib>

BEGIN_YAFARAY

struct PSample;
class ParamMap;
class Scene;

class GridVolumeRegion final : public DensityVolumeRegion
{
	public:
		static VolumeRegion *factory(Logger &logger, const ParamMap &params, const Scene &scene);
		~GridVolumeRegion() override;

	private:
		GridVolumeRegion(Logger &logger, Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax);
		virtual float density(Point3 p) const override;

		float ***grid_ = nullptr;
		int size_x_, size_y_, size_z_;
};

END_YAFARAY

#endif // YAFARAY_VOLUME_GRID_H