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

#ifndef LIBYAFARAY_LIGHT_SUN_H
#define LIBYAFARAY_LIGHT_SUN_H

#include "common/logger.h"
#include "light/light.h"
#include "geometry/vector.h"

namespace yafaray {

class ParamMap;
class Scene;

class SunLight final : public Light
{
		using ThisClassType_t = SunLight; using ParentClassType_t = Light;

	public:
		inline static std::string getClassName() { return "SunLight"; }
		static std::pair<std::unique_ptr<Light>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		SunLight(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items<Light> &lights);

	private:
		[[nodiscard]] Type type() const override { return Type::Sun; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(Vec3f, direction_, (Vec3f{{0.f, 0.f, 1.f}}), "direction", "");
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float, power_, 1.f, "power", "");
			PARAM_DECL(float, angle_, 0.27f, "angle", "Angular (half-)size of the real sun");
			PARAM_DECL(int, samples_, 4, "samples", "");
		} params_;
		size_t init(const Scene &scene) override;
		Rgb totalEnergy() const override { return color_ * e_pdf_; }
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		bool diracLight() const override { return false; }
		std::pair<bool, Ray> illumSample(const Point3f &surface_p, LSample &s, float time) const override;
		std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const override;
		bool canIntersect() const override { return true; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const override { return {}; }
		int nSamples() const override { return params_.samples_; }

		Point3f world_center_;
		const Rgb color_{params_.color_ * params_.power_};
		Rgb col_pdf_;
		const Vec3f direction_{params_.direction_.normalized()};
		const Uv<Vec3f> duv_{Vec3f::createCoordsSystem(params_.direction_)};
		float pdf_, invpdf_;
		float cos_angle_;
		float world_radius_;
		float e_pdf_;
};

} //namespace yafaray

#endif // LIBYAFARAY_LIGHT_SUN_H
