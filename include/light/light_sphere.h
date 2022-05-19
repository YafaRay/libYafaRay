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

#ifndef YAFARAY_LIGHT_SPHERE_H
#define YAFARAY_LIGHT_SPHERE_H

#include "light/light.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

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
	public:
		static Light *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		SphereLight(Logger &logger, const Point3 &c, float rad, const Rgb &col, float inte, int nsam, bool b_light_enabled = true, bool b_cast_shadows = true);
		void init(Scene &scene) override;
		Rgb totalEnergy() const override;
		Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		Rgb emitSample(Vec3 &wo, LSample &s, float time) const override;
		bool diracLight() const override { return false; }
		bool illumSample(const Point3 &surface_p, LSample &s, Ray &wi, float time) const override;
		bool illuminate(const Point3 &surface_p, Rgb &col, Ray &wi) const override { return false; }
		bool canIntersect() const override { return false; }
		bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const override;
		float illumPdf(const SurfacePoint &sp, const SurfacePoint &sp_light) const override;
		void emitPdf(const Vec3 &surface_n, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		int nSamples() const override { return samples_; }
		static bool sphereIntersect(const Ray &ray, const Point3 &c, float r_2, float &d_1, float &d_2);

		Point3 center_;
		float radius_, square_radius_, square_radius_epsilon_;
		Rgb color_; //!< includes intensity amplification! so...
		int samples_;
		std::string object_name_;
		float area_, inv_area_;
};

END_YAFARAY

#endif //YAFARAY_LIGHT_SPHERE_H
