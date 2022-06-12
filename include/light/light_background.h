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

#ifndef YAFARAY_LIGHT_BACKGROUND_H
#define YAFARAY_LIGHT_BACKGROUND_H

#include "light/light.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

class Background;
class Pdf1D;
class Scene;
class ParamMap;

class BackgroundLight final : public Light
{
	public:
		static Light *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		BackgroundLight(Logger &logger, int sampl, bool invert_intersect = false, bool light_enabled = true, bool cast_shadows = true);
		void init(Scene &scene) override;
		Rgb totalEnergy() const override;
		Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		Rgb emitSample(Vec3 &wo, LSample &s, float time) const override;
		bool diracLight() const override { return false; }
		bool illumSample(const Point3 &, LSample &s, Ray &wi, float time) const override;
		bool illuminate(const Point3 &surface_p, Rgb &col, Ray &wi) const override { return false; }
		float illumPdf(const Point3 &surface_p, const Point3 &light_p, const Vec3 &light_ng) const override;
		void emitPdf(const Vec3 &, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		int nSamples() const override { return samples_; }
		bool canIntersect() const override { return true; }
		std::tuple<bool, float, Rgb> intersect(const Ray &ray, float &t) const override;
		std::pair<Vec3, float> sampleDir(float s_1, float s_2, Vec3 &dir, float &NO, bool inv = false) const;
		float dirPdf(const Vec3 &dir) const;
		std::pair<float, Uv<float>> calcFromSample(float s_1, float s_2, bool inv = false) const;
		std::pair<float, Uv<float>> calcFromDir(const Vec3 &dir, bool inv) const;
		static float calcPdf(float p_0, float p_1, float s);
		static float calcInvPdf(float p_0, float p_1, float s);
		static constexpr float addOff(float v);
		static int clampSample(int s, int m);
		static float clampZero(float val);
		static float sinSample(float s);

		std::vector<std::unique_ptr<Pdf1D>> u_dist_;
		std::unique_ptr<Pdf1D> v_dist_;
		int samples_;
		Point3 world_center_;
		float world_radius_;
		float a_pdf_;
		float world_pi_factor_;
		bool abs_inter_;

		static constexpr int max_vsamples_ = 360;
		static constexpr int max_usamples_ = 720;
		static constexpr int min_samples_ = 16;

		static constexpr float smpl_off_ = 0.4999f;
		static constexpr float sigma_ = 0.000001f;
};

END_YAFARAY

#endif // YAFARAY_LIGHT_BACKGROUND_H
