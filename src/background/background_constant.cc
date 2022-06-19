/****************************************************************************
 *      background_constant.cc: a background using a constant color
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

#include "background/background_constant.h"
#include "common/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "image/image_output.h"

namespace yafaray {

ConstantBackground::ConstantBackground(Logger &logger, Rgb col) : Background(logger), color_(col)
{
}

Rgb ConstantBackground::eval(const Vec3 &dir, bool use_ibl_blur) const
{
	return color_;
}

const Background * ConstantBackground::factory(Logger &logger, Scene &scene, const std::string &name, const ParamMap &params)
{
	Rgb col(0.f);
	float power = 1.0;
	int ibl_sam = 16;
	bool ibl = false;
	bool cast_shadows = true;
	bool caus = true;
	bool diff = true;

	params.getParam("color", col);
	params.getParam("power", power);
	params.getParam("ibl", ibl);
	params.getParam("ibl_samples", ibl_sam);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", caus);
	params.getParam("with_diffuse", diff);

	auto const_bg = new ConstantBackground(logger, col * power);

	if(ibl)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = ibl_sam;
		bgp["with_caustic"] = caus;
		bgp["with_diffuse"] = diff;
		bgp["cast_shadows"] = cast_shadows;

		Light *bglight = scene.createLight("constantBackground_bgLight", std::move(bgp));
		bglight->setBackground(const_bg);
	}

	return const_bg;
}

} //namespace yafaray
