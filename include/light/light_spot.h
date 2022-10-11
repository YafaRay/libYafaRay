#pragma once
/****************************************************************************
 *      light_spot.h: a spot light with soft edge
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

#ifndef LIBYAFARAY_LIGHT_SPOT_H
#define LIBYAFARAY_LIGHT_SPOT_H

#include "light/light.h"
#include "geometry/vector.h"

namespace yafaray {

class Pdf1D;

class SpotLight final : public Light
{
		using ThisClassType_t = SpotLight; using ParentClassType_t = Light;

	public:
		inline static std::string getClassName() { return "SpotLight"; }
		static std::pair<std::unique_ptr<Light>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		SpotLight(Logger &logger, ParamError &param_error, const std::string &name, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::Spot; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Vec3f, from_, Vec3f{0.f}, "from", "");
			PARAM_DECL(Vec3f, to_, (Vec3f{{0.f, 0.f, -1.f}}), "to", "");
			PARAM_DECL(Rgb, color_, Rgb{1.f}, "color", "");
			PARAM_DECL(float, power_, 1.f, "power", "");
			PARAM_DECL(float, cone_angle_, 45.f, "cone_angle", "Cone angle in degrees");
			PARAM_DECL(float , falloff_, 0.15f, "blend", "");
			PARAM_DECL(bool , soft_shadows_, false, "soft_shadows", "");
			PARAM_DECL(float , shadow_fuzzyness_, 1.f, "shadowFuzzyness", "");
			PARAM_DECL(int , samples_, 8, "samples", "");
		} params_;
		Rgb totalEnergy() const override;
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const override;
		bool diracLight() const override { return !params_.soft_shadows_; }
		std::pair<bool, Ray> illumSample(const Point3f &surface_p, LSample &s, float time) const override;
		std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const override;
		std::array<float, 3> emitPdf(const Vec3f &surface_n, const Vec3f &wo) const override;
		bool canIntersect() const override { return params_.soft_shadows_; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		int nSamples() const override { return params_.samples_; };

		const Vec3f ndir_{(params_.from_ - params_.to_).normalize()}; //!< negative orientation (-dir)
		const Vec3f dir_{-ndir_}; //!< orientation of the spot cone
		const Uv<Vec3f> duv_{Vec3f::createCoordsSystem(dir_)}; //!< form a coordinate system with dir, to sample directions
		const Rgb color_{params_.color_ * params_.power_}; //<! color, premulitplied by light intensity
		float cos_start_, cos_end_; //<! cosStart is actually larger than cosEnd, because cos goes from +1 to -1
		float icos_diff_; //<! 1.0/(cosStart-cosEnd);
		std::unique_ptr<Pdf1D> pdf_;
		float interv_1_, interv_2_;
};

} //namespace yafaray

#endif // LIBYAFARAY_LIGHT_SPOT_H
