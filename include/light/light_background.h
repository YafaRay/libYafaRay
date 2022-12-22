#pragma once
/****************************************************************************
 *      bglight.h: a light source using the background
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

#ifndef LIBYAFARAY_LIGHT_BACKGROUND_H
#define LIBYAFARAY_LIGHT_BACKGROUND_H

#include "light/light.h"
#include "geometry/vector.h"

namespace yafaray {

class Background;
class Pdf1D;
class Scene;
class ParamMap;

class BackgroundLight final : public Light
{
		using ThisClassType_t = BackgroundLight; using ParentClassType_t = Light;

	public:
		inline static std::string getClassName() { return "BackgroundLight"; }
		static std::pair<std::unique_ptr<Light>, ParamResult> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		BackgroundLight(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items<Light> &lights);

	private:
		[[nodiscard]] Type type() const override { return Type::Background; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(int, samples_, 16, "samples", "");
			PARAM_DECL(bool, abs_intersect_, false, "abs_intersect", "");
			PARAM_DECL(float, ibl_clamp_sampling_, false, "ibl_clamp_sampling", "A value higher than 0.f 'clamps' the light intersection colors to that value, to reduce light sampling noise at the expense of realism and inexact overall light (0.f disables clamping)");
		} params_;
		void init(Scene &scene) override;
		Rgb totalEnergy() const override;
		std::tuple<Ray, float, Rgb> emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const override;
		std::pair<Vec3f, Rgb> emitSample(LSample &s, float time) const override;
		bool diracLight() const override { return false; }
		std::pair<bool, Ray> illumSample(const Point3f &, LSample &s, float time) const override;
		std::tuple<bool, Ray, Rgb> illuminate(const Point3f &surface_p, float time) const override;
		float illumPdf(const Point3f &surface_p, const Point3f &light_p, const Vec3f &light_ng) const override;
		std::array<float, 3> emitPdf(const Vec3f &, const Vec3f &wo) const override;
		int nSamples() const override { return params_.samples_; }
		bool canIntersect() const override { return true; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		std::pair<Vec3f, float> sampleDir(float s_1, float s_2, bool inv = false) const;
		float dirPdf(const Vec3f &dir) const;
		std::pair<float, Uv<float>> calcFromSample(float s_1, float s_2, bool inv = false) const;
		std::pair<float, Uv<float>> calcFromDir(const Vec3f &dir, bool inv) const;
		static float calcPdf(float p_0, float p_1, float s);
		static float calcInvPdf(float p_0, float p_1, float s);
		static constexpr inline float addOff(float v);
		static int clampSample(int s, int m);
		static float clampZero(float val);
		static float sinSample(float s);

		std::vector<std::unique_ptr<Pdf1D>> u_dist_;
		std::unique_ptr<Pdf1D> v_dist_;
		const Background *background_{nullptr};
		Point3f world_center_;
		float world_radius_;
		float a_pdf_;
		float world_pi_factor_;

		static constexpr inline int max_vsamples_ = 360;
		static constexpr inline int max_usamples_ = 720;
		static constexpr inline int min_samples_ = 16;

		static constexpr inline float smpl_off_ = 0.4999f;
		static constexpr inline float sigma_ = 0.000001f;
};

} //namespace yafaray

#endif // LIBYAFARAY_LIGHT_BACKGROUND_H
