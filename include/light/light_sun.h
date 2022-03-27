#pragma once
/****************************************************************************
 *      light_sun.h: a directional light with soft shadows
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#ifndef YAFARAY_LIGHT_SUN_H
#define YAFARAY_LIGHT_SUN_H

#include <common/logger.h>
#include "light/light.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

class ParamMap;
class Scene;

class SunLight final : public Light
{
	public:
		static Light *factory(Logger &logger, const ParamMap &params, const Scene &scene);

	private:
		SunLight(Logger &logger, Vec3 dir, const Rgb &col, float inte, float angle, int n_samples, bool b_light_enabled = true, bool b_cast_shadows = true);
		void init(Scene &scene) override;
		Rgb totalEnergy() const override { return color_ * e_pdf_; }
		Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		bool diracLight() const override { return false; }
		bool illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const override;
		bool illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const override { return false; }
		bool canIntersect() const override { return true; }
		bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const override;
		int nSamples() const override { return samples_; }

		Point3 world_center_;
		Rgb color_, col_pdf_;
		Vec3 direction_, du_, dv_;
		float pdf_, invpdf_;
		double cos_angle_;
		int samples_;
		float world_radius_;
		float e_pdf_;
};

END_YAFARAY

#endif // YAFARAY_LIGHT_SUN_H