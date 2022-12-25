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

#ifndef LIBYAFARAY_LIGHT_BACKGROUND_PORTAL_H
#define LIBYAFARAY_LIGHT_BACKGROUND_PORTAL_H

#include "light/light.h"
#include "geometry/vector.h"
#include <vector>

namespace yafaray {

class Primitive;
class Pdf1D;
class ParamMap;
class Scene;
class Background;
class Accelerator;
class Object;
template <typename T> class Items;

class BackgroundPortalLight final : public Light
{
		using ThisClassType_t = BackgroundPortalLight; using ParentClassType_t = Light;

	public:
		inline static std::string getClassName() { return "BackgroundPortalLight"; }
		static std::pair<std::unique_ptr<Light>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		BackgroundPortalLight(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map, const Items<Object> &objects, const Items<Light> &lights);

	private:
		[[nodiscard]] Type type() const override { return Type::BackgroundPortal; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(int, samples_, 16, "samples", "");
			PARAM_DECL(std::string, object_name_, "", "object_name", "");
			PARAM_DECL(float, power_, 1.f, "power", "");
			PARAM_DECL(float, ibl_clamp_sampling_, false, "ibl_clamp_sampling", "A value higher than 0.f 'clamps' the light intersection colors to that value, to reduce light sampling noise at the expense of realism and inexact overall light (0.f disables clamping)");
		} params_;
		size_t init(const Scene &scene) override;
		Rgb totalEnergy() const override;
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const override;
		bool diracLight() const override { return false; }
		//bool illumSample(const surfacePoint_t &sp, float s1, float s2, Rgb &col, float &ipdf, ray_t &wi) const override;
		std::pair<bool, Ray> illumSample(const Point3f &surface_p, LSample &s, float time) const override;
		std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const override;
		int nSamples() const override { return params_.samples_; }
		bool canIntersect() const override { return accelerator_ != nullptr /* false */ ; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		float illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &light_ng) const override;
		std::array<float, 3> emitPdf(const Vec3f &surface_n, const Vec3f &wi) const override;
		void initIs();
		std::pair<Point3f, Vec3f> sampleSurface(float s_1, float s_2, float time) const;

		size_t object_id_{math::invalid<size_t>};
		const Items<Object> &objects_;
		std::unique_ptr<Pdf1D> area_dist_;
		std::vector<const Primitive *> primitives_;
		const Background *background_{nullptr};
		float area_, inv_area_;
		std::unique_ptr<const Accelerator> accelerator_;
		Point3f world_center_;
};

} //namespace yafaray

#endif // Y_BACKGROUNDLIGHT_H
