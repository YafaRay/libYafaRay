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

#ifndef YAFARAY_VOLUME_SKY_H
#define YAFARAY_VOLUME_SKY_H

#include "volume/volume.h"

BEGIN_YAFARAY

struct PSample;
class ParamMap;
class Scene;

class SkyVolumeRegion final : public VolumeRegion
{
	public:
		static VolumeRegion *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		SkyVolumeRegion(Logger &logger, const Rgb &sa, const Rgb &ss, const Rgb &le, const Point3 &pmin, const Point3 &pmax);
		float p(const Vec3 &w_l, const Vec3 &w_s) const override;
		float phaseRayleigh(const Vec3 &w_l, const Vec3 &w_s) const;
		float phaseMie(const Vec3 &w_l, const Vec3 &w_s) const;
		Rgb sigmaA(const Point3 &p, const Vec3 &v) const override;
		Rgb sigmaS(const Point3 &p, const Vec3 &v) const override;
		Rgb emission(const Point3 &p, const Vec3 &v) const override;
		Rgb tau(const Ray &ray, float step, float offset) const override;

		Rgb s_ray_;
		Rgb s_mie_;
};

END_YAFARAY

#endif // YAFARAY_VOLUME_SKY_H