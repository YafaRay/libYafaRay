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

#include <memory>
#include "geometry/surface.h"
#include "sampler/sample.h"
#include "light/light_ies_data.h"
#include "geometry/ray.h"
#include "common/param.h"

namespace yafaray {

IesLight::IesLight(Logger &logger, const Point3 &from, const Point3 &to, const Rgb &col, float power, const std::string &ies_file, int smpls, bool s_sha, float ang, bool b_light_enabled, bool b_cast_shadows):
		Light(logger, Light::Flags::Singular), position_(from), samples_(smpls), soft_shadow_(s_sha)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	ies_data_ = std::make_unique<IesData>();

	if((ies_ok_ = ies_data_->parseIesFile(logger, ies_file)))
	{
		ndir_ = (from - to);
		ndir_.normalize();
		dir_ = -ndir_;

		duv_ = Vec3::createCoordsSystem(dir_);
		cos_end_ = math::cos(ies_data_->getMaxVAngle());

		color_ = col * power;
		tot_energy_ = math::mult_pi_by_2<> * (1.f - 0.5f * cos_end_);
	}
}

Uv<float> IesLight::getAngles(const Vec3 &dir, float costheta)
{
	float u = (dir.z() >= 1.f) ? 0.f : math::radToDeg(math::acos(dir.z()));
	if(dir.y() < 0) u = 360.f - u;
	const float v = (costheta >= 1.f) ? 0.f : math::radToDeg(math::acos(costheta));
	return {u, v};
}

std::tuple<bool, Ray, Rgb> IesLight::illuminate(const Point3 &surface_p, float time) const
{
	if(photonOnly()) return {};
	Vec3 ldir{position_ - surface_p};
	const float dist_sqrt = ldir.lengthSqr();
	const float dist = math::sqrt(dist_sqrt);
	const float i_dist_sqrt = 1.f / dist_sqrt;
	if(dist == 0.f) return {};
	ldir *= 1.f / dist; //normalize
	const float cos_a = ndir_ * ldir;
	if(cos_a < cos_end_) return {};
	const Uv<float> uv{getAngles(ldir, cos_a)};
	Rgb col{color_ * ies_data_->getRadiance(uv.u_, uv.v_) * i_dist_sqrt};
	Ray ray{surface_p, std::move(ldir), time, 0.f, dist};
	return {true, std::move(ray), std::move(col)};
}

std::pair<bool, Ray> IesLight::illumSample(const Point3 &surface_p, LSample &s, float time) const
{
	if(photonOnly()) return {};
	Vec3 ldir{position_ - surface_p};
	const float dist_sqrt = ldir.lengthSqr();
	const float dist = math::sqrt(dist_sqrt);
	const float i_dist_sqrt = 1.f / dist_sqrt;
	if(dist == 0.f) return {};
	ldir *= 1.f / dist; //normalize
	const float cos_a = ndir_ * ldir;
	if(cos_a < cos_end_) return {};
	Vec3 dir{sample::cone(ldir, duv_, cos_a, s.s_1_, s.s_2_)};
	const Uv<float> uv{getAngles(dir, cos_a)};
	const float rad = ies_data_->getRadiance(uv.u_, uv.v_);
	if(rad == 0.f) return {};
	s.col_ = color_ * i_dist_sqrt;
	s.pdf_ = 1.f / rad;
	Ray ray{surface_p, std::move(dir), time, 0.f, dist};
	return {true, std::move(ray)};
}

std::tuple<Ray, float, Rgb> IesLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, float time) const
{
	Vec3 dir{sample::cone(dir_, duv_, cos_end_, s_1, s_2)};
	float cos_a = dir * dir_;
	if(cos_a < cos_end_) return {};
	const Uv<float> uv{getAngles(dir, cos_a)};
	const float rad = ies_data_->getRadiance(uv.u_, uv.v_);
	Ray ray{position_, std::move(dir), time};
	return {std::move(ray), rad, color_};
}

std::pair<Vec3, Rgb> IesLight::emitSample(LSample &s, float time) const
{
	s.sp_->p_ = position_;
	s.flags_ = flags_;
	Vec3 dir{sample::cone(dir_, duv_, cos_end_, s.s_3_, s.s_4_)};
	const Uv<float> uv{getAngles(dir, dir * dir_)};
	const float rad = ies_data_->getRadiance(uv.u_, uv.v_);
	s.dir_pdf_ = (rad > 0.f) ? (tot_energy_ / rad) : 0.f;
	s.area_pdf_ = 1.f;
	return {std::move(dir), color_ * rad * tot_energy_};
}

std::array<float, 3> IesLight::emitPdf(const Vec3 &surface_n, const Vec3 &wo) const
{
	const float cos_wo = 1.f;
	const float area_pdf = 1.f;
	float dir_pdf = 0.f;
	const float cos_a = dir_ * wo;
	if(cos_a >= cos_end_)
	{
		const Uv<float> uv{getAngles(wo, cos_a)};
		float rad = ies_data_->getRadiance(uv.u_, uv.v_);
		dir_pdf = (rad > 0.f) ? (tot_energy_ / rad) : 0.f;
	}
	return {area_pdf, dir_pdf, cos_wo};
}

Light * IesLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3 from{0.f, 0.f, 0.f};
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

	auto light = new IesLight(logger, from, to, color, power, file, sam, s_sha, ang, light_enabled, cast_shadows);

	if(!light->isIesOk()) return nullptr;

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

} //namespace yafaray
