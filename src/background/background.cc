/****************************************************************************
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

#include "background/background.h"
#include "background/background_darksky.h"
#include "background/background_gradient.h"
#include "background/background_texture.h"
#include "background/background_constant.h"
#include "background/background_sunsky.h"
#include "common/param.h"
#include "common/logger.h"

BEGIN_YAFARAY

std::unique_ptr<Background> Background::factory(Logger &logger, ParamMap &params, Scene &scene)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Background");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	if(type == "darksky") return DarkSkyBackground::factory(logger, params, scene);
	else if(type == "gradientback") return GradientBackground::factory(logger, params, scene);
	else if(type == "sunsky") return SunSkyBackground::factory(logger, params, scene);
	else if(type == "textureback") return TextureBackground::factory(logger, params, scene);
	else if(type == "constant") return ConstantBackground::factory(logger, params, scene);
	else return nullptr;
}

END_YAFARAY
