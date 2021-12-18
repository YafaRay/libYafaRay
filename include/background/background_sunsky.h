#pragma once
/****************************************************************************
 *      background_sunsky.h: a light source using the background
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

#ifndef YAFARAY_BACKGROUND_SUNSKY_H
#define YAFARAY_BACKGROUND_SUNSKY_H

#include "background.h"
#include "color/color.h"
#include "geometry/vector.h"

BEGIN_YAFARAY

// sunsky, from 'A Practical Analytic Model For DayLight" by Preetham, Shirley & Smits.
// http://www.cs.utah.edu/vissim/papers/sunsky/
// based on the actual code by Brian Smits
// and a thread on gamedev.net on skycolor algorithms

class SunSkyBackground final : public Background
{
	public:
		static std::unique_ptr<Background> factory(Logger &logger, ParamMap &params, Scene &scene);
		virtual ~SunSkyBackground() override;

	private:
		SunSkyBackground(Logger &logger, const Point3 dir, float turb, float a_var, float b_var, float c_var, float d_var, float e_var, float pwr, bool ibl, bool with_caustic);
		virtual Rgb operator()(const Vec3 &dir, bool use_ibl_blur = false) const override;
		virtual Rgb eval(const Vec3 &dir, bool use_ibl_blur = false) const override;
		Rgb getSkyCol(const Vec3 &dir) const;
		static Rgb computeAttenuatedSunlight(float theta, int turbidity);

		Vec3 sun_dir_;
		double theta_s_, phi_s_;	// sun coords
		double theta_2_, theta_3_, t_, t_2_;
		double zenith_Y_, zenith_x_, zenith_y_;
		double perez_Y_[5], perez_x_[5], perez_y_[5];
		double angleBetween(double thetav, double phiv) const;
		double perezFunction(const double *lam, double theta, double gamma, double lvz) const;
		float power_;
};

END_YAFARAY

#endif // YAFARAY_BACKGROUND_SUNSKY_H