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

namespace yafaray {

class Scene;
class ParamMap;

class AreaLight final : public Light
{
	public:
		static Light *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		AreaLight(Logger &logger, const Point3f &c, const Vec3f &v_1, const Vec3f &v_2, const Rgb &col, float inte, int nsam, bool light_enabled = true, bool cast_shadows = true);
		void init(Scene &scene) override;
		Rgb totalEnergy() const override;
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const override;
		bool diracLight() const override { return false; }
		std::pair<bool, Ray> illumSample(const Point3f &surface_p, LSample &s, float time) const override;
		std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const override;
		bool canIntersect() const override { return true; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		float illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &light_ng) const override;
		std::array<float, 3> emitPdf(const Vec3f &surface_n, const Vec3f &wo) const override;
		int nSamples() const override { return samples_; }
		static bool triIntersect(const Point3f &a, const Point3f &b, const Point3f &c, const Ray &ray, float &t);

		Point3f corner_, c_2_, c_3_, c_4_;
		Vec3f to_x_, to_y_, normal_, fnormal_;
		Uv<Vec3f> duv_; //!< directions for hemisampler (emitting photons)
		Rgb color_; //!< includes intensity amplification! so...
		int samples_;
		std::string object_name_;
		float area_, inv_area_;
};

} //namespace yafaray

#endif //YAFARAY_LIGHT_AREA_H
