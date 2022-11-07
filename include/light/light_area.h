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

#ifndef LIBYAFARAY_LIGHT_AREA_H
#define LIBYAFARAY_LIGHT_AREA_H

#include "common/logger.h"
#include "light/light.h"
#include "geometry/vector.h"
#include "geometry/shape/shape_polygon.h"

namespace yafaray {

class Scene;
class ParamMap;

class AreaLight final : public Light
{
		using ThisClassType_t = AreaLight; using ParentClassType_t = Light;

	public:
		inline static std::string getClassName() { return "AreaLight"; }
		static std::pair<std::unique_ptr<Light>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		AreaLight(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const SceneItems<Light> &lights);

	private:
		[[nodiscard]] Type type() const override { return Type::Area; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Vec3f, corner_, Vec3f{0.f}, "corner", "");
			PARAM_DECL(Vec3f, point_1_, Vec3f{0.f}, "point1", "");
			PARAM_DECL(Vec3f, point_2_, Vec3f{0.f}, "point2", "");
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float , power_, 1.f, "power", "");
			PARAM_DECL(int, samples_, 4, "samples", "");
			PARAM_DECL(std::string, object_name_, "", "object_name", "");
		} params_;
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
		int nSamples() const override { return params_.samples_; }

		const ShapePolygon<float, 4> area_quad_{{params_.corner_, params_.point_1_, params_.point_1_ + params_.point_2_ - params_.corner_, params_.point_2_}};
		const Vec3f to_x_{params_.point_1_ - params_.corner_};
		const Vec3f to_y_{params_.point_2_ - params_.corner_};
		const Vec3f normal_flipped_{(params_.point_2_ - params_.corner_) ^ (params_.point_1_ - params_.corner_)};
		const Vec3f normal_{-normal_flipped_};
		const Uv<Vec3f> duv_{(params_.point_1_ - params_.corner_).normalized(), normal_ ^ (params_.point_1_ - params_.corner_).normalized()}; //!< directions for hemisampler (emitting photons)
		const Rgb color_{params_.color_ * params_.power_ * math::num_pi<>}; //!< includes intensity amplification! so...
		const float area_{normal_flipped_.normalized().length()};
		const float inv_area_{1.f / area_};
};

} //namespace yafaray

#endif //LIBYAFARAY_LIGHT_AREA_H
