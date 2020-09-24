/****************************************************************************
 *      sunsky.cc: a light source using the background
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

#include "background/background_sunsky.h"
#include "color/spectrum_sun.h"
#include "common/logging.h"
#include "common/param.h"
#include "scene/scene.h"
#include "light/light.h"

BEGIN_YAFARAY

// sunsky, from 'A Practical Analytic Model For DayLight" by Preetham, Shirley & Smits.
// http://www.cs.utah.edu/vissim/papers/sunsky/
// based on the actual code by Brian Smits
// and a thread on gamedev.net on skycolor algorithms


SunSkyBackground::SunSkyBackground(const Point3 dir, float turb, float a_var, float b_var, float c_var, float d_var, float e_var, float pwr, bool ibl, bool with_caustic): power_(pwr)
{
	with_ibl_ = ibl;
	shoot_caustic_ = with_caustic;

	sun_dir_.set(dir.x_, dir.y_, dir.z_);
	sun_dir_.normalize();
	theta_s_ = math::acos(sun_dir_.z_);
	theta_2_ = theta_s_ * theta_s_;
	theta_3_ = theta_2_ * theta_s_;
	phi_s_ = atan2(sun_dir_.y_, sun_dir_.x_);
	t_ = turb;
	t_2_ = turb * turb;
	double chi = (4.0 / 9.0 - t_ / 120.0) * (M_PI - 2.0 * theta_s_);
	zenith_Y_ = (4.0453 * t_ - 4.9710) * tan(chi) - 0.2155 * t_ + 2.4192;
	zenith_Y_ *= 1000;  // conversion from kcd/m^2 to cd/m^2

	zenith_x_ = (0.00165 * theta_3_ - 0.00375 * theta_2_ + 0.00209 * theta_s_) * t_2_ +
			   (-0.02903 * theta_3_ + 0.06377 * theta_2_ - 0.03202 * theta_s_ + 0.00394) * t_ +
				(0.11693 * theta_3_ - 0.21196 * theta_2_ + 0.06052 * theta_s_ + 0.25886);

	zenith_y_ = (0.00275 * theta_3_ - 0.00610 * theta_2_ + 0.00317 * theta_s_) * t_2_ +
				(-0.04214 * theta_3_ + 0.08970 * theta_2_ - 0.04153 * theta_s_ + 0.00516) * t_ +
				(0.15346 * theta_3_ - 0.26756 * theta_2_ + 0.06670 * theta_s_ + 0.26688);

	perez_Y_[0] = (0.17872 * t_ - 1.46303) * a_var;
	perez_Y_[1] = (-0.35540 * t_ + 0.42749) * b_var;
	perez_Y_[2] = (-0.02266 * t_ + 5.32505) * c_var;
	perez_Y_[3] = (0.12064 * t_ - 2.57705) * d_var;
	perez_Y_[4] = (-0.06696 * t_ + 0.37027) * e_var;

	perez_x_[0] = (-0.01925 * t_ - 0.25922) * a_var;
	perez_x_[1] = (-0.06651 * t_ + 0.00081) * b_var;
	perez_x_[2] = (-0.00041 * t_ + 0.21247) * c_var;
	perez_x_[3] = (-0.06409 * t_ - 0.89887) * d_var;
	perez_x_[4] = (-0.00325 * t_ + 0.04517) * e_var;

	perez_y_[0] = (-0.01669 * t_ - 0.26078) * a_var;
	perez_y_[1] = (-0.09495 * t_ + 0.00921) * b_var;
	perez_y_[2] = (-0.00792 * t_ + 0.21023) * c_var;
	perez_y_[3] = (-0.04405 * t_ - 1.65369) * d_var;
	perez_y_[4] = (-0.01092 * t_ + 0.05291) * e_var;
}

SunSkyBackground::~SunSkyBackground()
{
	// Empty
}

double SunSkyBackground::perezFunction(const double *lam, double theta, double gamma, double lvz) const
{
	double e_1 = 0, e_2 = 0, e_3 = 0, e_4 = 0;
	if(lam[1] <= 230.)
		e_1 = math::exp(lam[1]);
	else
		e_1 = 7.7220185e99;
	if((e_2 = lam[3] * theta_s_) <= 230.)
		e_2 = math::exp(e_2);
	else
		e_2 = 7.7220185e99;
	if((e_3 = lam[1] / cos(theta)) <= 230.)
		e_3 = math::exp(e_3);
	else
		e_3 = 7.7220185e99;
	if((e_4 = lam[3] * gamma) <= 230.)
		e_4 = math::exp(e_4);
	else
		e_4 = 7.7220185e99;
	double den = (1 + lam[0] * e_1) * (1 + lam[2] * e_2 + lam[4] * math::cos(theta_s_) * math::cos(theta_s_));
	double num = (1 + lam[0] * e_3) * (1 + lam[2] * e_4 + lam[4] * math::cos(gamma) * math::cos(gamma));
	return (lvz * num / den);
}


double SunSkyBackground::angleBetween(double thetav, double phiv) const
{
	double cospsi = math::sin(thetav) * math::sin(theta_s_) * math::cos(phi_s_ - phiv) + math::cos(thetav) * math::cos(theta_s_);
	if(cospsi > 1)  return 0;
	if(cospsi < -1) return M_PI;
	return math::acos(cospsi);
}

inline Rgb SunSkyBackground::getSkyCol(const Ray &ray) const
{
	Vec3 iw = ray.dir_;
	iw.normalize();

	double theta, phi, hfade = 1, nfade = 1;

	Rgb skycolor(0.0);

	theta = math::acos(iw.z_);
	if(theta > (0.5 * M_PI))
	{
		// this stretches horizon color below horizon, must be possible to do something better...
		// to compensate, simple fade to black
		hfade = 1.0 - (theta * M_1_PI - 0.5) * 2.0;
		hfade = hfade * hfade * (3.0 - 2.0 * hfade);
		theta = 0.5 * M_PI;
	}
	// compensation for nighttime exaggerated blue
	// also simple fade out towards zenith
	if(theta_s_ > (0.5 * M_PI))
	{
		if(theta <= 0.5 * M_PI)
		{
			nfade = 1.0 - (0.5 - theta * M_1_PI) * 2.0;
			nfade *= 1.0 - (theta_s_ * M_1_PI - 0.5) * 2.0;
			nfade = nfade * nfade * (3.0 - 2.0 * nfade);
		}
	}

	if((iw.y_ == 0.0) && (iw.x_ == 0.0))
		phi = M_PI * 0.5;
	else
		phi = atan2(iw.y_, iw.x_);

	double gamma = angleBetween(theta, phi);
	// Compute xyY values
	double x = perezFunction(perez_x_, theta, gamma, zenith_x_);
	double y = perezFunction(perez_y_, theta, gamma, zenith_y_);
	// Luminance scale 1.0/15000.0
	double Y = 6.666666667e-5 * nfade * hfade * perezFunction(perez_Y_, theta, gamma, zenith_Y_);

	if(y == 0.f) return skycolor;

	// conversion to RGB, from gamedev.net thread on skycolor computation
	double X = (x / y) * Y;
	double z = ((1.0 - x - y) / y) * Y;

	skycolor.set((3.240479 * X - 1.537150 * Y - 0.498535 * z),
	             (-0.969256 * X + 1.875992 * Y + 0.041556 * z),
	             (0.055648 * X - 0.204043 * Y + 1.057311 * z));
	skycolor.clampRgb01();
	return skycolor;
}

Rgb SunSkyBackground::operator()(const Ray &ray, RenderState &state, bool from_postprocessed) const
{
	return power_ * getSkyCol(ray);
}

Rgb SunSkyBackground::eval(const Ray &ray, bool from_postprocessed) const
{
	return power_ * getSkyCol(ray);
}

Background *SunSkyBackground::factory(ParamMap &params, Scene &scene)
{
	Point3 dir(1, 1, 1);	// same as sunlight, position interpreted as direction
	float turb = 4.0;	// turbidity of atmosphere
	bool add_sun = false;	// automatically add real sunlight
	bool bgl = false;
	int bgl_samples = 8;
	double power = 1.0;
	float pw = 1.0;	// sunlight power
	float av, bv, cv, dv, ev;
	av = bv = cv = dv = ev = 1.0;	// color variation parameters, default is normal
	bool cast_shadows = true;
	bool cast_shadows_sun = true;
	bool caus = true;
	bool diff = true;

	params.getParam("from", dir);
	params.getParam("turbidity", turb);
	params.getParam("power", power);

	// new color variation parameters
	params.getParam("a_var", av);
	params.getParam("b_var", bv);
	params.getParam("c_var", cv);
	params.getParam("d_var", dv);
	params.getParam("e_var", ev);

	// create sunlight with correct color and position?
	params.getParam("add_sun", add_sun);
	params.getParam("sun_power", pw);

	params.getParam("background_light", bgl);
	params.getParam("light_samples", bgl_samples);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("cast_shadows_sun", cast_shadows_sun);

	params.getParam("with_caustic", caus);
	params.getParam("with_diffuse", diff);

	Background *new_sunsky = new SunSkyBackground(dir, turb, av, bv, cv, dv, ev, power, bgl, true);

	if(bgl)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = bgl_samples;
		bgp["cast_shadows"] = cast_shadows;
		bgp["with_caustic"] = caus;
		bgp["with_diffuse"] = diff;

		Light *bglight = scene.createLight("sunsky_bgLight", bgp);
		bglight->setBackground(new_sunsky);
	}

	if(add_sun)
	{
		Rgb suncol = computeAttenuatedSunlight__(math::acos(std::fabs(dir.z_)), turb);//(*new_sunsky)(vector3d_t(dir.x, dir.y, dir.z));
		double angle = 0.27;
		double cos_angle = cos(math::degToRad(angle));
		float invpdf = (2.f * M_PI * (1.f - cos_angle));
		suncol *= invpdf * power;

		Y_VERBOSE << "Sunsky: sun color = " << suncol << YENDL;

		ParamMap p;
		p["type"] = std::string("sunlight");
		p["direction"] = Point3(dir[0], dir[1], dir[2]);
		p["color"] = suncol;
		p["angle"] = Parameter(angle);
		p["power"] = Parameter(pw);
		p["cast_shadows"] = cast_shadows_sun;
		p["with_caustic"] = caus;
		p["with_diffuse"] = diff;

		scene.createLight("sunsky_SUN", p);
	}

	return new_sunsky;
}

END_YAFARAY
