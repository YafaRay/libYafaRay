/****************************************************************************
 *      light_ies.cc: IES Light
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

#include "light/light_ies.h"
#include "geometry/surface.h"
#include "sampler/sample.h"
#include "light/light_ies_data.h"
#include "geometry/ray.h"
#include "common/param.h"

BEGIN_YAFARAY

IesLight::IesLight(Logger &logger, const Point3 &from, const Point3 &to, const Rgb &col, float power, const std::string ies_file, int smpls, bool s_sha, float ang, bool b_light_enabled, bool b_cast_shadows):
		Light(logger, Light::Flags::Singular), position_(from), samples_(smpls), soft_shadow_(s_sha)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	ies_data_ = std::unique_ptr<IesData>(new IesData());

	if((ies_ok_ = ies_data_->parseIesFile(logger, ies_file)))
	{
		ndir_ = (from - to);
		ndir_.normalize();
		dir_ = -ndir_;

		Vec3::createCs(dir_, du_, dv_);
		cos_end_ = math::cos(ies_data_->getMaxVAngle());

		color_ = col * power;
		tot_energy_ = math::mult_pi_by_2 * (1.f - 0.5f * cos_end_);
	}
}

void IesLight::getAngles(float &u, float &v, const Vec3 &dir, const float &costheta) const
{
	u = (dir.z_ >= 1.f) ? 0.f : math::radToDeg(math::acos(dir.z_));

	if(dir.y_ < 0)
	{
		u = 360.f - u;
	}

	v = (costheta >= 1.f) ? 0.f : math::radToDeg(math::acos(costheta));
}

bool IesLight::illuminate(const SurfacePoint &sp, Rgb &col, Ray &wi) const
{
	if(photonOnly()) return false;

	Vec3 ldir(position_ - sp.p_);
	float dist_sqrt = ldir.lengthSqr();
	float dist = math::sqrt(dist_sqrt);
	float i_dist_sqrt = 1.f / dist_sqrt;

	if(dist == 0.0) return false;

	ldir *= 1.f / dist; //normalize

	float cosa = ndir_ * ldir;
	if(cosa < cos_end_) return false;

	float u, v;

	getAngles(u, v, ldir, cosa);

	col = color_ * ies_data_->getRadiance(u, v) * i_dist_sqrt;

	wi.tmax_ = dist;
	wi.dir_ = ldir;

	return true;
}

bool IesLight::illumSample(const SurfacePoint &sp, LSample &s, Ray &wi) const
{
	if(photonOnly()) return false;

	Vec3 ldir(position_ - sp.p_);
	float dist_sqrt = ldir.lengthSqr();
	float dist = math::sqrt(dist_sqrt);
	float i_dist_sqrt = 1.f / dist_sqrt;

	if(dist == 0.0) return false;

	ldir *= 1.f / dist; //normalize

	float cosa = ndir_ * ldir;
	if(cosa < cos_end_) return false;

	wi.tmax_ = dist;
	wi.dir_ = sample::cone(ldir, du_, dv_, cosa, s.s_1_, s.s_2_);

	float u, v;
	getAngles(u, v, wi.dir_, cosa);

	float rad = ies_data_->getRadiance(u, v);

	if(rad == 0.f) return false;

	s.col_ = color_ * i_dist_sqrt;
	s.pdf_ = 1.f / rad;

	return true;
}

bool IesLight::canIntersect() const
{
	return false;
}

bool IesLight::intersect(const Ray &ray, float &t, Rgb &col, float &ipdf) const
{
	return false;
}

Rgb IesLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	ray.from_ = position_;
	ray.dir_ = sample::cone(dir_, du_, dv_, cos_end_, s_1, s_2);

	ipdf = 0.f;

	float cosa = ray.dir_ * dir_;

	if(cosa < cos_end_) return Rgb(0.f);

	float u, v;
	getAngles(u, v, ray.dir_, cosa);

	float rad = ies_data_->getRadiance(u, v);

	ipdf = rad;

	return color_;
}

Rgb IesLight::emitSample(Vec3 &wo, LSample &s) const
{
	s.sp_->p_ = position_;
	s.flags_ = flags_;

	wo = sample::cone(dir_, du_, dv_, cos_end_, s.s_3_, s.s_4_);

	float u, v;
	getAngles(u, v, wo, wo * dir_);

	float rad = ies_data_->getRadiance(u, v);

	s.dir_pdf_ = (rad > 0.f) ? (tot_energy_ / rad) : 0.f;
	s.area_pdf_ = 1.f;

	return color_ * rad * tot_energy_;
}

void IesLight::emitPdf(const SurfacePoint &sp, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	cos_wo = 1.f;
	area_pdf = 1.f;
	dir_pdf = 0.f;

	float cosa = dir_ * wo;

	if(cosa < cos_end_) return;

	float u, v;
	getAngles(u, v, wo, cosa);

	float rad = ies_data_->getRadiance(u, v);

	dir_pdf = (rad > 0.f) ? (tot_energy_ / rad) : 0.f;
}

std::unique_ptr<Light> IesLight::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	Point3 from(0.0);
	Point3 to(0.f, 0.f, -1.f);
	Rgb color(1.0);
	float power = 1.0;
	std::string file;
	int sam = 16; //wild goose... sorry guess :D
	bool s_sha = false;
	float ang = 180.f; //full hemi
	bool light_enabled = true;
	bool cast_shadows = true;
	bool shoot_d = true;
	bool shoot_c = true;
	bool p_only = false;

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("file", file);
	params.getParam("samples", sam);
	params.getParam("soft_shadows", s_sha);
	params.getParam("cone_angle", ang);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);
	params.getParam("photon_only", p_only);

	auto light = std::unique_ptr<IesLight>(new IesLight(logger, from, to, color, power, file, sam, s_sha, ang, light_enabled, cast_shadows));

	if(!light->isIesOk()) return nullptr;

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
