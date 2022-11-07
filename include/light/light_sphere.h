#pragma once
/****************************************************************************
 *      light_sphere.h: a spherical area light source
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

#ifndef LIBYAFARAY_LIGHT_SPHERE_H
#define LIBYAFARAY_LIGHT_SPHERE_H

#include "light/light.h"
#include "geometry/vector.h"

namespace yafaray {

class ParamMap;
class Scene;

/*! sphere lights are *drumroll* lights with a sphere shape.
	They only emit light on the outside! On the inside it is somewhat pointless,
	because in that case, sampling from BSDF would _always_ be at least as efficient,
	and in most cases much smarter, so use just geometry with emiting material...
	The light samples from the cone in which the light is visible, instead of directly
	from the sphere surface (thanks to PBRT for the hint)
*/

class SphereLight final : public Light
{
		using ThisClassType_t = SphereLight; using ParentClassType_t = Light;

	public:
		inline static std::string getClassName() { return "SphereLight"; }
		static std::pair<std::unique_ptr<Light>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		SphereLight(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const SceneItems<Light> &lights);

	private:
		[[nodiscard]] Type type() const override { return Type::Sphere; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Vec3f, from_, Vec3f{0.f}, "from", "");
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float, power_, 1.f, "power", "");
			PARAM_DECL(float, radius_, 1.f, "radius", "");
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
		bool canIntersect() const override { return false; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		float illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &light_ng) const override;
		std::array<float, 3> emitPdf(const Vec3f &surface_n, const Vec3f &wo) const override;
		int nSamples() const override { return params_.samples_; }
		static std::pair<bool, Uv<float>> sphereIntersect(const Point3f &from, const Vec3f &dir, const Point3f &c, float r_2);

		const float square_radius_{params_.radius_ * params_.radius_};
		const float square_radius_epsilon_{static_cast<float>(static_cast<double>(square_radius_) * 1.000003815)}; // ~0.2% larger radius squared
		const Rgb color_{params_.color_ * params_.power_}; //!< includes intensity amplification! so...
		const float area_{square_radius_ * 4.f * math::num_pi<>};
		const float inv_area_{1.f / area_};
};

} //namespace yafaray

#endif //LIBYAFARAY_LIGHT_SPHERE_H
