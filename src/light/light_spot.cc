/****************************************************************************
 *      light_spot.cc: a spot light with soft edge
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

#include "light/light_spot.h"

#include <memory>
#include "geometry/surface.h"
#include "geometry/ray.h"
#include "sampler/sample.h"
#include "sampler/sample_pdf1d.h"
#include "common/param.h"

BEGIN_YAFARAY

SpotLight::SpotLight(Logger &logger, const Point3 &from, const Point3 &to, const Rgb &col, float power, float angle, float falloff, bool s_sha, int smpl, float ssfuzzy, bool b_light_enabled, bool b_cast_shadows):
		Light(logger, Light::Flags::Singular), position_(from), soft_shadows_(s_sha), shadow_fuzzy_(ssfuzzy), samples_(smpl)
{
	light_enabled_ = b_light_enabled;
	cast_shadows_ = b_cast_shadows;
	ndir_ = (from - to).normalize();
	dir_ = -ndir_;
	color_ = col * power;
	duv_ = Vec3::createCoordsSystem(dir_);
	double rad_angle = math::degToRad(angle);
	double rad_inner_angle = rad_angle * (1.f - falloff);
	cos_start_ = math::cos(rad_inner_angle);
	cos_end_ = math::cos(rad_angle);
	icos_diff_ = 1.f / (cos_start_ - cos_end_);

	std::vector<float> f(65);
	for(int i = 0; i < 65; i++)
	{
		const float v = static_cast<float>(i) / 64.f;
		f[i] = v * v * (3.f - 2.f * v);
	}
	pdf_ = std::make_unique<Pdf1D>(f);

	/* the integral of the smoothstep is 0.5, and since it gets applied to the cos, and each delta cos
		corresponds to a constant surface are of the (partial) emitting sphere, we can actually simply
		compute the energie emitted from both areas, the constant and blending one...
		1  cosStart  cosEnd              -1
		|------|--------|-----------------|
	*/

	interv_1_ = 1.f - cos_start_;
	interv_2_ = 0.5f * (cos_start_ - cos_end_); // as said, energy linear to delta cos, integral is 0.5;
	float sum = std::abs(interv_1_) + std::abs(interv_2_);
	if(sum > 0.f) sum = 1.f / sum;
	interv_1_ *= sum;
	interv_2_ *= sum;
}

Rgb SpotLight::totalEnergy() const
{
	return color_ * math::mult_pi_by_2<> * (1.f - 0.5f * (cos_start_ + cos_end_));
}

bool SpotLight::illuminate(const Point3 &surface_p, Rgb &col, Ray &wi) const
{
	if(photonOnly()) return false;

	Vec3 ldir{position_ - surface_p};
	float dist_sqr = ldir * ldir;
	float dist = math::sqrt(dist_sqr);
	if(dist == 0.0) return false;

	float idist_sqr = 1.f / dist_sqr;
	ldir *= 1.f / dist; //normalize

	float cosa = ndir_ * ldir;

	if(cosa < cos_end_) return false; //outside cone
	if(cosa >= cos_start_) // not affected by falloff
	{
		col = color_ * idist_sqr;
	}
	else
	{
		float v = (cosa - cos_end_) * icos_diff_;
		v = v * v * (3.f - 2.f * v);
		col = color_ * v * idist_sqr;
	}

	wi.tmax_ = dist;
	wi.dir_ = ldir;
	return true;
}

bool SpotLight::illumSample(const Point3 &surface_p, LSample &s, Ray &wi, float time) const
{
	if(photonOnly()) return false;

	Vec3 ldir{position_ - surface_p};
	float dist_sqr = ldir * ldir;
	if(dist_sqr == 0.0) return false;
	float dist = math::sqrt(dist_sqr);

	ldir *= 1.f / dist; //normalize

	float cosa = ndir_ * ldir;
	if(cosa < cos_end_) return false; //outside cone

	wi.tmax_ = dist;
	wi.dir_ = sample::cone(ldir, duv_.u_, duv_.v_, cos_end_, s.s_1_ * shadow_fuzzy_, s.s_2_ * shadow_fuzzy_);

	if(cosa >= cos_start_) // not affected by falloff
	{
		s.col_ = color_;
	}
	else
	{
		float v = (cosa - cos_end_) * icos_diff_;
		v = v * v * (3.f - 2.f * v);
		s.col_ = color_ * v;
	}

	s.flags_ = flags_;
	s.pdf_ = dist_sqr;

	//FIXME: I don't quite understand how pdf is calculated in the spotlight, but something looks wrong when dist<1.f, so I'm applying a manual correction to keep s.pdf >= 1 always, and "move" the effect of the distance to the color itself. This is a horrible patch but at least solves a problem causing darker light when distance between spotlight and surface is less than 1.f
	if(s.pdf_ < 1.f)
	{
		s.pdf_ = 1.f;
		s.col_ = s.col_ / dist_sqr;
	}

	return true;
}

Rgb SpotLight::emitPhoton(float s_1, float s_2, float s_3, float s_4, Ray &ray, float &ipdf) const
{
	ray.from_ = position_;

	if(s_3 <= interv_1_) // sample from cone not affected by falloff:
	{
		ray.dir_ = sample::cone(dir_, duv_.u_, duv_.v_, cos_start_, s_1, s_2);
		ipdf = math::mult_pi_by_2<> * (1.f - cos_start_) / interv_1_;
	}
	else // sample in the falloff area
	{
		auto[sm_2, spdf]{pdf_->sample(s_2)};
		sm_2 *= pdf_->invSize();
		ipdf = math::mult_pi_by_2<> * (cos_start_ - cos_end_) / (interv_2_ * spdf);
		double cos_ang = cos_end_ + (cos_start_ - cos_end_) * (double)sm_2;
		double sin_ang = math::sqrt(1.0 - cos_ang * cos_ang);
		float t_1 = math::mult_pi_by_2<> * s_1;
		ray.dir_ = (duv_.u_ * math::cos(t_1) + duv_.v_ * math::sin(t_1)) * (float)sin_ang + dir_ * (float)cos_ang;
		return color_ * spdf * pdf_->integral(); // scale is just the actual falloff function, since spdf is func * invIntegral...
	}
	return color_;
}

Rgb SpotLight::emitSample(Vec3 &wo, LSample &s, float time) const
{
	s.sp_->p_ = position_;
	s.area_pdf_ = 1.f;
	s.flags_ = flags_;
	if(s.s_3_ <= interv_1_) // sample from cone not affected by falloff:
	{
		wo = sample::cone(dir_, duv_.u_, duv_.v_, cos_start_, s.s_1_, s.s_2_);
		s.dir_pdf_ = interv_1_ / (math::mult_pi_by_2<> * (1.f - cos_start_));
	}
	else // sample in the falloff area
	{
		auto[sm_2, spdf]{pdf_->sample(s.s_2_)};
		sm_2 *= pdf_->invSize();
		s.dir_pdf_ = (interv_2_ * spdf) / (math::mult_pi_by_2<> * (cos_start_ - cos_end_));
		double cos_ang = cos_end_ + (cos_start_ - cos_end_) * (double)sm_2;
		double sin_ang = math::sqrt(1.0 - cos_ang * cos_ang);
		float t_1 = math::mult_pi_by_2<> * s.s_1_;
		wo = (duv_.u_ * math::cos(t_1) + duv_.v_ * math::sin(t_1)) * (float)sin_ang + dir_ * (float)cos_ang;
		float v = sm_2 * sm_2 * (3.f - 2.f * sm_2);
		return color_ * v;
	}
	return color_;
}

void SpotLight::emitPdf(const Vec3 &surface_n, const Vec3 &wo, float &area_pdf, float &dir_pdf, float &cos_wo) const
{
	area_pdf = 1.f;
	cos_wo = 1.f;

	float cosa = dir_ * wo;
	if(cosa < cos_end_) dir_pdf = 0.f;
	else if(cosa >= cos_start_) // not affected by falloff
	{
		dir_pdf = interv_1_ / (math::mult_pi_by_2<> * (1.f - cos_start_));
	}
	else
	{
		float v = (cosa - cos_end_) * icos_diff_;
		v = v * v * (3.f - 2.f * v);
		dir_pdf = interv_2_ * v * 2.f / (math::mult_pi_by_2<> * (cos_start_ - cos_end_));   //divide by integral of v (0.5)?
	}
}
std::tuple<bool, float, Rgb> SpotLight::intersect(const Ray &ray, float &t) const
{
	const float cos_a = dir_ * ray.dir_;
	if(cos_a == 0.f) return {};
	t = (dir_ * (position_ - ray.from_)) / cos_a;
	if(t < 0.f) return {};
	const Point3 p{ray.from_ + t * ray.dir_};
	if(dir_ * (p - position_) == 0.f)
	{
		if(p * p <= 1e-2f)
		{
			const float cosa = dir_ * ray.dir_;
			if(cosa < cos_end_) return {}; //outside cone
			Rgb col{color_};
			if(cosa < cos_start_) //affected by falloff
			{
				float v = (cosa - cos_end_) * icos_diff_;
				v = v * v * (3.f - 2.f * v);
				col *= v;
			}
			const float ipdf = 1.f / (t * t);
			if(logger_.isVerbose()) logger_.logVerbose("SpotLight: ipdf, color = ", ipdf, ", ", color_);
			return {true, ipdf, col};
		}
	}
	return {};
}

Light * SpotLight::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Point3 from{0.f, 0.f, 0.f};
	Point3 to(0.f, 0.f, -1.f);
	Rgb color(1.0);
	float power = 1.0;
	float angle = 45, falloff = 0.15;
	bool p_only = false;
	bool soft_shadows = false;
	int smpl = 8;
	float ssfuzzy = 1.f;
	bool light_enabled = true;
	bool cast_shadows = true;
	bool shoot_d = true;
	bool shoot_c = true;

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("cone_angle", angle);
	params.getParam("blend", falloff);
	params.getParam("photon_only", p_only);
	params.getParam("soft_shadows", soft_shadows);
	params.getParam("shadowFuzzyness", ssfuzzy);
	params.getParam("samples", smpl);
	params.getParam("light_enabled", light_enabled);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", shoot_c);
	params.getParam("with_diffuse", shoot_d);

	auto light = new SpotLight(logger, from, to, color, power, angle, falloff, soft_shadows, smpl, ssfuzzy, light_enabled, cast_shadows);

	light->shoot_caustic_ = shoot_c;
	light->shoot_diffuse_ = shoot_d;
	light->photon_only_ = p_only;

	return light;
}

END_YAFARAY
