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

BEGIN_YAFARAY

class RenderData;
struct PSample;
class ParamMap;
class Scene;

class ExpDensityVolumeRegion final : public DensityVolumeRegion
{
	public:
		static std::unique_ptr<VolumeRegion> factory(const ParamMap &params, const Scene &scene);

	private:
		ExpDensityVolumeRegion(Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax, int attgrid_scale, float aa, float bb);
		virtual float density(Point3 p) const override;

		float a_, b_;
};

END_YAFARAY

#endif // YAFARAY_VOLUME_EXP_DENSITY_H