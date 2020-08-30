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

#ifndef YAFARAY_BGLIGHT_H
#define YAFARAY_BGLIGHT_H

#include <core_api/light.h>
#include <core_api/environment.h>
#include <core_api/vector3d.h>

BEGIN_YAFRAY

class Background;
class Pdf1D;

class BackgroundLight final : public Light
{
	public:
		static Light *factory(ParamMap &params, RenderEnvironment &render);

	private:
		BackgroundLight(int sampl, bool invert_intersect = false, bool light_enabled = true, bool cast_shadows = true);
		virtual ~BackgroundLight() override;
		virtual void init(Scene &scene) override;
		virtual Rgb totalEnergy() const override;
		virtual Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		virtual Rgb emitSample(Vec3 &wo, LSample &s) const override;
		virtual bool diracLight() const override { return false; }
		virtual bool illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const override;
		virtual bool illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const override { return false; }
		virtual float illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const override;
		virtual void emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		virtual int nSamples() const override { return samples_; }
		virtual bool canIntersect() const override { return true; }
		virtual bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const override;
		void sampleDir(float s_1, float s_2, Vec3 &dir, float &pdf, bool inv = false) const;
		float dirPdf(const Vec3 dir) const;
		float calcFromSample(float s_1, float s_2, float &u, float &v, bool inv = false) const;
		float calcFromDir(const Vec3 &dir, float &u, float &v, bool inv = false) const;

		Pdf1D **u_dist_ = nullptr, *v_dist_ = nullptr;
		int samples_;
		Point3 world_center_;
		float world_radius_;
		float a_pdf_, ia_pdf_;
		float world_pi_factor_;
		bool abs_inter_;
};

END_YAFRAY

#endif // YAFARAY_BGLIGHT_H
