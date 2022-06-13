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

#ifndef YAFARAY_LIGHT_AREA_H
#define YAFARAY_LIGHT_AREA_H

#include <common/logger.h>
#include "light/light.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

class Scene;
class ParamMap;

class AreaLight final : public Light
{
	public:
		static Light *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		AreaLight(Logger &logger, const Point3 &c, const Vec3 &v_1, const Vec3 &v_2, const Rgb &col, float inte, int nsam, bool light_enabled = true, bool cast_shadows = true);
		void init(Scene &scene) override;
		Rgb totalEnergy() const override;
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		std::pair<Vec3, Rgb> emitSample(LSample &s, float time) const override;
		bool diracLight() const override { return false; }
		bool illumSample(const Point3 &surface_p, LSample &s, Ray &wi, float time) const override;
		bool illuminate(const Point3 &surface_p, Rgb &col, Ray &wi) const override { return false; }
		bool canIntersect() const override { return true; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		float illumPdf(const Point3 &surface_p, const Point3 &light_p, const Vec3 &light_ng) const override;
		void emitPdf(const Vec3 &surface_n, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		int nSamples() const override { return samples_; }
		static bool triIntersect(const Point3 &a, const Point3 &b, const Point3 &c, const Ray &ray, float &t);

		Point3 corner_, c_2_, c_3_, c_4_;
		Vec3 to_x_, to_y_, normal_, fnormal_;
		Uv<Vec3> duv_; //!< directions for hemisampler (emitting photons)
		Rgb color_; //!< includes intensity amplification! so...
		int samples_;
		std::string object_name_;
		float area_, inv_area_;
};

END_YAFARAY

#endif //YAFARAY_LIGHT_AREA_H
