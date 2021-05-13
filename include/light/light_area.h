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

#ifndef YAFARAY_LIGHT_AREA_H
#define YAFARAY_LIGHT_AREA_H

#include "light/light.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

class Scene;
class ParamMap;

class AreaLight final : public Light
{
	public:
		static std::unique_ptr<Light> factory(ParamMap &params, const Scene &scene);

	private:
		AreaLight(const Point3 &c, const Vec3 &v_1, const Vec3 &v_2,
				  const Rgb &col, float inte, int nsam, bool light_enabled = true, bool cast_shadows = true);
		virtual void init(Scene &scene) override;
		virtual Rgb totalEnergy() const override;
		virtual Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		virtual Rgb emitSample(Vec3 &wo, LSample &s) const override;
		virtual bool diracLight() const override { return false; }
		virtual bool illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const override;
		virtual bool illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const override { return false; }
		virtual bool canIntersect() const override { return true; }
		virtual bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const override;
		virtual float illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const override;
		virtual void emitPdf(const SurfacePoint &sp, const Vec3 &wi, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		virtual int nSamples() const override { return samples_; }

		Point3 corner_, c_2_, c_3_, c_4_;
		Vec3 to_x_, to_y_, normal_, fnormal_;
		Vec3 du_, dv_; //!< directions for hemisampler (emitting photons)
		Rgb color_; //!< includes intensity amplification! so...
		int samples_;
		std::string object_name_;
		float area_, inv_area_;
};

END_YAFARAY

#endif //YAFARAY_LIGHT_AREA_H
