/****************************************************************************
 *      background_gradient.cc: a background using a simple color gradient
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

#include "background/background_gradient.h"
#include "common/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "image/image_output.h"
#include "geometry/vector.h"

namespace yafaray {

GradientBackground::GradientBackground(Logger &logger, const Rgb &gzcol, const Rgb &ghcol, const Rgb &szcol, const Rgb &shcol) :
		Background(logger), gzenith_(gzcol), ghoriz_(ghcol), szenith_(szcol), shoriz_(shcol)
{
}

Rgb GradientBackground::eval(const Vec3f &dir, bool use_ibl_blur) const
{
	float blend = dir[Axis::Z];
	Rgb color;
	if(blend >= 0.f) color = blend * szenith_ + (1.f - blend) * shoriz_;
	else
	{
		blend = -blend;
		color = blend * gzenith_ + (1.f - blend) * ghoriz_;
	}
	if(color.minimum() < 1e-6f) color = Rgb(1e-5f);
	return color;
}

const Background * GradientBackground::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	Rgb gzenith, ghoriz, szenith(0.4f, 0.5f, 1.f), shoriz(1.f);
	float p = 1.0;
	bool bgl = false;
	int bgl_sam = 16;
	bool cast_shadows = true;
	bool caus = true;
	bool diff = true;

	params.getParam("horizon_color", shoriz);
	params.getParam("zenith_color", szenith);
	gzenith = szenith;
	ghoriz = shoriz;
	params.getParam("horizon_ground_color", ghoriz);
	params.getParam("zenith_ground_color", gzenith);
	params.getParam("ibl", bgl);
	params.getParam("ibl_samples", bgl_sam);
	params.getParam("power", p);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", caus);
	params.getParam("with_diffuse", diff);

	auto grad_bg = new GradientBackground(logger, gzenith * p, ghoriz * p, szenith * p, shoriz * p);

	if(bgl)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = bgl_sam;
		bgp["with_caustic"] = caus;
		bgp["with_diffuse"] = diff;
		bgp["cast_shadows"] = cast_shadows;

		std::unique_ptr<Light> bglight{Light::factory(logger, scene, "light", std::move(bgp))};
		bglight->setBackground(grad_bg);
		grad_bg->addLight(std::move(bglight));
	}
	return grad_bg;
}

} //namespace yafaray
