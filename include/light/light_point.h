#pragma once
/****************************************************************************
 *      light_point.h: a simple point light source
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

#ifndef YAFARAY_LIGHT_POINT_H
#define YAFARAY_LIGHT_POINT_H

#include <common/logger.h>
#include "light/light.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

class ParamMap;
class Scene;

class PointLight final : public Light
{
	public:
		static Light *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		PointLight(Logger &logger, const Point3 &pos, const Rgb &col, float inte, bool b_light_enabled = true, bool b_cast_shadows = true);
		Rgb totalEnergy() const override { return color_ * 4.0f * math::num_pi<>; }
		Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		Rgb emitSample(Vec3 &wo, LSample &s, float time) const override;
		bool diracLight() const override { return true; }
		bool illumSample(const SurfacePoint &sp, LSample &s, Ray &wi, float time) const override;
		bool illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const override;
		void emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const override;

		Point3 position_;
		Rgb color_;
		float intensity_;
};

END_YAFARAY

#endif // YAFARAY_LIGHT_POINT_H


