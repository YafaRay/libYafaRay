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

#ifndef YAFARAY_LIGHT_SPOT_H
#define YAFARAY_LIGHT_SPOT_H

#include "light/light.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

class Pdf1D;

class SpotLight final : public Light
{
	public:
		static Light *factory(Logger &logger, const ParamMap &params, const Scene &scene);

	private:
		SpotLight(Logger &logger, const Point3 &from, const Point3 &to, const Rgb &col, float power, float angle, float falloff, bool s_sha, int smpl, float ssfuzzy, bool b_light_enabled = true, bool b_cast_shadows = true);
		Rgb totalEnergy() const override;
		Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		Rgb emitSample(Vec3 &wo, LSample &s) const override;
		bool diracLight() const override { return !soft_shadows_; }
		bool illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const override;
		bool illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const override;
		void emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		bool canIntersect() const override { return soft_shadows_; }
		bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const override;
		int nSamples() const override { return samples_; };

		Point3 position_;
		Vec3 dir_; //!< orientation of the spot cone
		Vec3 ndir_; //!< negative orientation (-dir)
		Vec3 du_, dv_; //!< form a coordinate system with dir, to sample directions
		float cos_start_, cos_end_; //<! cosStart is actually larger than cosEnd, because cos goes from +1 to -1
		float icos_diff_; //<! 1.0/(cosStart-cosEnd);
		Rgb color_; //<! color, premulitplied by light intensity
		std::unique_ptr<Pdf1D> pdf_;
		float interv_1_, interv_2_;

		bool soft_shadows_;
		float shadow_fuzzy_;
		int samples_;
};

END_YAFARAY

#endif // YAFARAY_LIGHT_SPOT_H