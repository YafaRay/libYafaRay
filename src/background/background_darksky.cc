/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      darksky.cc: SkyLight, "Real" Sunlight and Sky Background
 *      Created on: 20/03/2009
 *
 *      Based on the original implementation by Alejandro Conty (jandro), Mathias Wein (Lynx), anyone else???
 *      Actual implementation by Rodrigo Placencia (Darktide)
 *
 *      Based on 'A Practical Analytic Model For DayLight" by Preetham, Shirley & Smits.
 *      http://www.cs.utah.edu/vissim/papers/sunsky/
 *      based on the actual code by Brian Smits
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

#include "background/background_darksky.h"
#include "common/logger.h"
#include "common/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "image/image_output.h"
#include "color/spectral_data.h"
#include "math/interpolation_curve.h"

BEGIN_YAFARAY

DarkSkyBackground::DarkSkyBackground(Logger &logger, const Point3 dir, float turb, float pwr, float sky_bright, bool clamp, float av, float bv, float cv, float dv, float ev, float altitude, bool night, float exp, bool genc, ColorConv::ColorSpace cs, bool ibl, bool with_caustic):
		Background(logger), power_(pwr * sky_bright), sky_brightness_(sky_bright), color_conv_(clamp, genc, cs, exp), alt_(altitude), night_sky_(night)
{
	std::string act;

	with_ibl_ = ibl;
	shoot_caustic_ = with_caustic;

	sun_dir_ = Vec3(dir);
	sun_dir_.z_ += alt_;
	sun_dir_.normalize();

	theta_s_ = math::acos(sun_dir_.z_);

	act = (night_sky_) ? "ON" : "OFF";
	if(logger_.isVerbose())
	{
		logger_.logVerbose("DarkSky: Night mode [ ", act, " ]");
		logger_.logVerbose("DarkSky: Solar Declination in Degrees (", math::radToDeg(theta_s_), ")");
		act = (clamp) ? "active." : "inactive.";
		logger_.logVerbose("DarkSky: RGB Clamping ", act);
		logger_.logVerbose("DarkSky: Altitude ", alt_);
	}

	cos_theta_s_ = math::cos(theta_s_);
	cos_theta_2_ = cos_theta_s_ * cos_theta_s_;
	sin_theta_s_ = math::sin(theta_s_);

	theta_2_ = theta_s_ * theta_s_;
	theta_3_ = theta_2_ * theta_s_;

	t_ = turb;
	t_2_ = turb * turb;

	const double chi = (0.44444444 - (t_ / 120.0)) * (math::num_pi - (2.0 * theta_s_));

	zenith_Y_ = (4.0453 * t_ - 4.9710) * std::tan(chi) - 0.2155 * t_ + 2.4192;
	zenith_Y_ *= 1000;  // conversion from kcd/m^2 to cd/m^2

	zenith_x_ =
			(0.00165 * theta_3_ - 0.00374 * theta_2_ + 0.00209 * theta_s_) * t_2_ +
			(-0.02902 * theta_3_ + 0.06377 * theta_2_ - 0.03202 * theta_s_ + 0.00394) * t_ +
			(0.11693 * theta_3_ - 0.21196 * theta_2_ + 0.06052 * theta_s_ + 0.25885);

	zenith_y_ =
			(0.00275 * theta_3_ - 0.00610 * theta_2_ + 0.00316 * theta_s_) * t_2_ +
			(-0.04214 * theta_3_ + 0.08970 * theta_2_ - 0.04153 * theta_s_ + 0.00515) * t_ +
			(0.15346 * theta_3_ - 0.26756 * theta_2_ + 0.06669 * theta_s_ + 0.26688);

	perez_Y_[0] = ((0.17872 * t_) - 1.46303) * av;
	perez_Y_[1] = ((-0.35540 * t_) + 0.42749) * bv;
	perez_Y_[2] = ((-0.02266 * t_) + 5.32505) * cv;
	perez_Y_[3] = ((0.12064 * t_) - 2.57705) * dv;
	perez_Y_[4] = ((-0.06696 * t_) + 0.37027) * ev;
	perez_Y_[5] = prePerez(perez_Y_);

	perez_x_[0] = ((-0.01925 * t_) - 0.25922);
	perez_x_[1] = ((-0.06651 * t_) + 0.00081);
	perez_x_[2] = ((-0.00041 * t_) + 0.21247);
	perez_x_[3] = ((-0.06409 * t_) - 0.89887);
	perez_x_[4] = ((-0.00325 * t_) + 0.04517);
	perez_x_[5] = prePerez(perez_x_);

	perez_y_[0] = ((-0.01669 * t_) - 0.26078);
	perez_y_[1] = ((-0.09495 * t_) + 0.00921);
	perez_y_[2] = ((-0.00792 * t_) + 0.21023);
	perez_y_[3] = ((-0.04405 * t_) - 1.65369);
	perez_y_[4] = ((-0.01092 * t_) + 0.05291);
	perez_y_[5] = prePerez(perez_y_);
}

Rgb DarkSkyBackground::getAttenuatedSunColor()
{
	Rgb light_color(1.0);
	light_color = getSunColorFromSunRad();
	if(night_sky_) light_color *= Rgb(0.8, 0.8, 1.0);
	return light_color;
}

Rgb DarkSkyBackground::getSunColorFromSunRad()
{
	const double b = (0.04608365822050 * t_) - 0.04586025928522;
	const double a = 1.3;
	const double l = 0.35;
	const double w = 2.0;

	const IrregularCurve ko(spectral_data::ko_amplitudes.data(), spectral_data::ko_wavelengths.data(), spectral_data::ko_amplitudes.size());
	const IrregularCurve kg(spectral_data::kg_amplitudes.data(), spectral_data::kg_wavelengths.data(), spectral_data::kg_amplitudes.size());
	const IrregularCurve kwa(spectral_data::kwa_amplitudes.data(), spectral_data::kwa_wavelengths.data(), spectral_data::kwa_amplitudes.size());
	const RegularCurve sun_radiance_curve(spectral_data::sun_radiance.data(), 380, 750, spectral_data::sun_radiance.size());

	const double m = 1.0 / (cos_theta_s_ + 0.15 * std::pow(93.885f - math::radToDeg(theta_s_), -1.253f));
	const double mw = m * w;
	const double lm = -m * l;

	const double m_1 = -0.008735;
	const double m_b = -b;
	const double am = -a * m;
	const double m_4 = -4.08 * m;

	Rgb s_xyz(0.0);
	Rgb spdf(0.0);
	for(int wavelength = 380; wavelength < 750; wavelength += 5)
	{
		const double u_l = wavelength * 0.001;
		const double kg_lm = kg(wavelength) * m;
		const double kwa_lmw = kwa(wavelength) * mw;

		const double rayleigh = std::exp(m_1 * std::pow(u_l, m_4));
		const double angstrom = std::exp(m_b * std::pow(u_l, am));
		const double ozone = std::exp(ko(wavelength) * lm);
		const double gas = std::exp((-1.41 * kg_lm) / std::pow(1 + 118.93 * kg_lm, 0.45));
		const double water = std::exp((-0.2385 * kwa_lmw) / std::pow(1 + 20.07 * kwa_lmw, 0.45));
		spdf = sun_radiance_curve(wavelength) * rayleigh * angstrom * ozone * gas * water;
		s_xyz += spectral_data::chromaMatch(wavelength) * spdf * 0.013513514;
	}
	return color_conv_.fromXyz(s_xyz, true);
}

double DarkSkyBackground::prePerez(const double *perez)
{
	const double p_num = ((1 + perez[0] * std::exp(perez[1])) * (1 + (perez[2] * std::exp(perez[3] * theta_s_)) + (perez[4] * cos_theta_2_)));
	if(p_num == 0.0) return 0.0;
	return 1.0 / p_num;
}

double DarkSkyBackground::perezFunction(const double *lam, double cos_theta, double gamma, double cos_gamma, double lvz) const
{
	const double num = ((1 + lam[0] * std::exp(lam[1] / cos_theta)) * (1 + lam[2] * std::exp(lam[3] * gamma) + lam[4] * cos_gamma));
	return lvz * num * lam[5];
}

inline Rgb DarkSkyBackground::getSkyCol(const Vec3 &dir) const
{
	Vec3 iw {dir};
	iw.z_ += alt_;
	iw.normalize();

	double cos_theta = iw.z_;
	if(cos_theta <= 0.0) cos_theta = 1e-6;
	double cos_gamma = iw * sun_dir_;
	const double cos_gamma_2 = cos_gamma * cos_gamma;
	const double gamma = std::acos(cos_gamma);

	const double x = perezFunction(perez_x_, cos_theta, gamma, cos_gamma_2, zenith_x_);
	const double y = perezFunction(perez_y_, cos_theta, gamma, cos_gamma_2, zenith_y_);
	const double Y = perezFunction(perez_Y_, cos_theta, gamma, cos_gamma_2, zenith_Y_) * 6.66666667e-5;

	Rgb sky_col = color_conv_.fromxyY(x, y, Y);
	if(night_sky_) sky_col *= Rgb(0.05, 0.05, 0.08);
	return sky_col * sky_brightness_;
}

Rgb DarkSkyBackground::operator()(const Vec3 &dir, bool from_postprocessed) const
{
	return getSkyCol(dir);
}

Rgb DarkSkyBackground::eval(const Vec3 &dir, bool from_postprocessed) const
{
	return getSkyCol(dir) * power_;
}

std::unique_ptr<Background> DarkSkyBackground::factory(Logger &logger, ParamMap &params, Scene &scene)
{
	Point3 dir(1, 1, 1);
	float turb = 4.0;
	float altitude = 0.0;
	int bgl_samples = 8;
	float power = 1.0; //bgLight Power
	float pw = 1.0;// sunLight power
	float bright = 1.0;
	bool add_sun = false;
	bool bgl = false;
	bool clamp = true;
	bool night = false;
	float av, bv, cv, dv, ev;
	bool caus = true;
	bool diff = true;
	av = bv = cv = dv = ev = 1.0;
	bool gamma_enc = true;
	std::string cs = "CIE (E)";
	float exp = 1.f;
	bool cast_shadows = true;
	bool cast_shadows_sun = true;

	if(logger.isVerbose()) logger.logVerbose("DarkSky: Begin");

	params.getParam("from", dir);
	params.getParam("turbidity", turb);
	params.getParam("altitude", altitude);
	params.getParam("power", power);
	params.getParam("bright", bright);

	//params.getParam("clamp_rgb", clamp);
	params.getParam("exposure", exp);
	//params.getParam("gamma_enc", gammaEnc);
	params.getParam("color_space", cs);

	params.getParam("a_var", av); //Darkening or brightening towards horizon
	params.getParam("b_var", bv); //Luminance gradient near the horizon
	params.getParam("c_var", cv); //Relative intensity of circumsolar region
	params.getParam("d_var", dv); //Width of circumsolar region
	params.getParam("e_var", ev); //Relative backscattered light

	params.getParam("add_sun", add_sun);
	params.getParam("sun_power", pw);

	params.getParam("background_light", bgl);
	params.getParam("with_caustic", caus);
	params.getParam("with_diffuse", diff);
	params.getParam("light_samples", bgl_samples);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("cast_shadows_sun", cast_shadows_sun);

	params.getParam("night", night);

	ColorConv::ColorSpace color_s = ColorConv::CieRgbECs;
	if(cs == "CIE (E)") color_s = ColorConv::CieRgbECs;
	else if(cs == "CIE (D50)") color_s = ColorConv::CieRgbD50Cs;
	else if(cs == "sRGB (D65)") color_s = ColorConv::SRgbD65Cs;
	else if(cs == "sRGB (D50)") color_s = ColorConv::SRgbD50Cs;

	if(night)
	{
		bright *= 0.5;
		pw *= 0.5;
	}

	auto dark_sky = std::unique_ptr<DarkSkyBackground>(new DarkSkyBackground(logger, dir, turb, power, bright, clamp, av, bv, cv, dv, ev, altitude, night, exp, gamma_enc, color_s, bgl, caus));

	if(add_sun && math::radToDeg(math::acos(dir.z_)) < 100.0)
	{
		Vec3 d(dir);
		d.normalize();

		Rgb suncol = dark_sky->getAttenuatedSunColor();
		double angle = 0.5 * (2.0 - d.z_);

		if(logger.isVerbose()) logger.logVerbose("DarkSky: SunColor = ", suncol);

		ParamMap p;
		p["type"] = std::string("sunlight");
		p["direction"] = Point3(d);
		p["color"] = suncol;
		p["angle"] = Parameter(angle);
		p["power"] = Parameter(pw);
		p["samples"] = bgl_samples;
		p["cast_shadows"] = cast_shadows_sun;
		p["with_caustic"] = caus;
		p["with_diffuse"] = diff;

		if(logger.isVerbose()) logger.logVerbose("DarkSky: Adding a \"Real Sun\"");

		scene.createLight("DarkSky_RealSun", p);
	}

	if(bgl)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = bgl_samples;
		bgp["with_caustic"] = caus;
		bgp["with_diffuse"] = diff;
		bgp["cast_shadows"] = cast_shadows;

		if(logger.isVerbose()) logger.logVerbose("DarkSky: Adding background light");

		Light *bglight = scene.createLight("DarkSky_bgLight", bgp);

		bglight->setBackground(dark_sky.get());
	}

	if(logger.isVerbose()) logger.logVerbose("DarkSky: End");

	return dark_sky;
}

END_YAFARAY
