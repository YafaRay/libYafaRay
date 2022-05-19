#pragma once
/****************************************************************************
 *      light_ies.h: IES Light
 *      This is part of the libYafaRay package
 *      Copyright (C) 2009  Bert Buchholz and Rodrigo Placencia
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

#ifndef YAFARAY_LIGHT_IES_H
#define YAFARAY_LIGHT_IES_H

#include <common/logger.h>
#include "light/light.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

class ParamMap;
class Scene;
class IesData;

class IesLight final : public Light
{
	public:
		static Light *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		IesLight(Logger &logger, const Point3 &from, const Point3 &to, const Rgb &col, float power, const std::string &ies_file, int smpls, bool s_sha, float ang, bool b_light_enabled = true, bool b_cast_shadows = true);
		Rgb totalEnergy() const override{ return color_ * tot_energy_;};
		int nSamples() const override { return samples_; };
		bool diracLight() const override { return !soft_shadow_; }
		bool illuminate(const Point3 &surface_p, Rgb &col, Ray &wi) const override;
		bool illumSample(const Point3 &surface_p, LSample &s, Ray &wi, float time) const override;
		bool canIntersect() const override;
		bool intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const override;
		Rgb emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const override;
		Rgb emitSample(Vec3 &wo, LSample &s, float time) const override;
		void emitPdf(const Vec3 &surface_n, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const override;
		bool isIesOk() const { return ies_ok_; };
		static void getAngles(float &u, float &v, const Vec3 &dir, float costheta);

		Point3 position_;
		Vec3 dir_; //!< orientation of the spot cone
		Vec3 ndir_; //!< negative orientation (-dir)
		Vec3 du_, dv_; //!< form a coordinate system with dir, to sample directions
		float cos_end_; //<! cosStart is actually larger than cosEnd, because cos goes from +1 to -1
		Rgb color_; //<! color, premulitplied by light intensity
		int samples_;
		bool soft_shadow_;
		float tot_energy_;
		std::unique_ptr<IesData> ies_data_;
		bool ies_ok_;
};

END_YAFARAY

#endif // YAFARAY_LIGHT_IES_H
