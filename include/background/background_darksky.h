/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      background_darksky.h: SkyLight, "Real" Sunlight and Sky Background
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

#ifndef YAFARAY_BACKGROUND_DARKSKY_H
#define YAFARAY_BACKGROUND_DARKSKY_H

#include <memory>
#include "background.h"
#include "geometry/vector.h"
#include "color/color_conversion.h"

namespace yafaray {

class DarkSkyBackground final : public Background
{
	public:
		static const Background * factory(Logger &logger, Scene &scene, const std::string &name, const ParamMap &params);

	private:
		DarkSkyBackground(Logger &logger, const Point3f &dir, float turb, float pwr, float sky_bright, bool clamp, float av, float bv, float cv, float dv, float ev, float altitude, bool night, float exp, bool genc, ColorConv::ColorSpace cs);
		Rgb operator()(const Vec3f &dir, bool use_ibl_blur) const override;
		Rgb eval(const Vec3f &dir, bool use_ibl_blur) const override;
		Rgb getAttenuatedSunColor();
		Rgb getSkyCol(const Vec3f &dir) const;
		double prePerez(const std::array<double, 6> &perez) const;
		Rgb getSunColorFromSunRad();
		static double perezFunction(const std::array<double, 6> &lam, double cos_theta, double gamma, double cos_gamma, double lvz);

		Vec3f sun_dir_;
		double theta_s_;
		double theta_2_, theta_3_;
		double cos_theta_s_, cos_theta_2_;
		double t_, t_2_;
		double zenith_Y_, zenith_x_, zenith_y_;
		std::array<double, 6> perez_Y_, perez_x_, perez_y_;
		float power_;
		float sky_brightness_;
		ColorConv color_conv_;
		float alt_;
		bool night_sky_;
};

} //namespace yafaray

#endif // YAFARAY_BACKGROUND_DARKSKY_H