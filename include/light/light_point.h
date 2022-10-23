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

#ifndef LIBYAFARAY_LIGHT_POINT_H
#define LIBYAFARAY_LIGHT_POINT_H

#include "common/logger.h"
#include "light/light.h"
#include "geometry/vector.h"

namespace yafaray {

class ParamMap;
class Scene;

class PointLight final : public Light
{
		using ThisClassType_t = PointLight; using ParentClassType_t = Light;

	public:
		inline static std::string getClassName() { return "PointLight"; }
		static std::pair<std::unique_ptr<Light>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		PointLight(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::Point; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Vec3f, from_, Vec3f{0.f}, "from", "");
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float, power_, 1.f, "power", "");
		} params_;
		Rgb totalEnergy() const override { return color_ * 4.0f * math::num_pi<>; }
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const override;
		bool diracLight() const override { return true; }
		std::pair<bool, Ray> illumSample(const Point3f &surface_p, LSample &s, float time) const override;
		std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const override;
		std::array<float, 3> emitPdf(const Vec3f &, const Vec3f &wo) const override;

		const Rgb color_{params_.color_ * params_.power_};
};

} //namespace yafaray

#endif // LIBYAFARAY_LIGHT_POINT_H


