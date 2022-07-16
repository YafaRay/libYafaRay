#pragma once
/****************************************************************************
 *      bgportallight.h: a light source using a triangle mesh as portal
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

#ifndef YAFARAY_LIGHT_BACKGROUND_PORTAL_H
#define YAFARAY_LIGHT_BACKGROUND_PORTAL_H

#include "light/light.h"
#include "geometry/vector.h"
#include <vector>

namespace yafaray {

class Object;
class Primitive;
class Pdf1D;
class ParamMap;
class Scene;
class Background;
class Accelerator;

class BackgroundPortalLight final : public Light
{
	public:
		static Light *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		BackgroundPortalLight(Logger &logger, std::string object_name, int sampl, float pow, bool light_enabled = true, bool cast_shadows = true);
		void init(const Scene &scene) override;
		Rgb totalEnergy() const override;
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const override;
		bool diracLight() const override { return false; }
		//bool illumSample(const surfacePoint_t &sp, float s1, float s2, Rgb &col, float &ipdf, ray_t &wi) const override;
		std::pair<bool, Ray> illumSample(const Point3f &surface_p, LSample &s, float time) const override;
		std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const override;
		int nSamples() const override { return samples_; }
		bool canIntersect() const override { return accelerator_ != nullptr /* false */ ; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		float illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &light_ng) const override;
		std::array<float, 3> emitPdf(const Vec3f &surface_n, const Vec3f &wi) const override;
		void initIs();
		std::pair<Point3f, Vec3f> sampleSurface(float s_1, float s_2, float time) const;

		std::string object_name_;
		std::unique_ptr<Pdf1D> area_dist_;
		std::vector<const Primitive *> primitives_;
		int samples_;
		int num_primitives_; //!< gives the array size of uDist
		float area_, inv_area_;
		float power_;
		Object *base_object_ = nullptr;
		std::unique_ptr<const Accelerator> accelerator_;
		const Background *bg_ = nullptr;
		Point3f world_center_;
		float a_pdf_;
};

} //namespace yafaray

#endif // Y_BACKGROUNDLIGHT_H
