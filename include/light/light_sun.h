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

#include "light/light.h"
#include "common/vector.h"

BEGIN_YAFARAY

class ParamMap;
class RenderEnvironment;

class SunLight final : public Light
{
	public:
		static Light *factory(ParamMap &params, RenderEnvironment &render);

	private:
		SunLight(Vec3 dir, const Rgb &col, float inte, float angle, int n_samples, bool b_light_enabled = true, bool b_cast_shadows = true);
		virtual void init(Scene &scene);
		virtual Rgb totalEnergy() const { return color_ * e_pdf_; }
		virtual Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const;
		virtual bool diracLight() const { return false; }
		virtual bool illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const;
		virtual bool illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const { return false; }
		virtual bool canIntersect() const { return true; }
		virtual bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const;
		virtual int nSamples() const { return samples_; }

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