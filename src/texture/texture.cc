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

#include "texture/texture.h"
#include "texture/texture_basic.h"
#include "texture/texture_image.h"
#include "common/param.h"

BEGIN_YAFARAY

Texture *Texture::factory(ParamMap &params, Scene &scene)
{
	std::string type;
	params.getParam("type", type);
	if(type == "blend") return BlendTexture::factory(params, scene);
	else if(type == "clouds") return CloudsTexture::factory(params, scene);
	else if(type == "marble") return MarbleTexture::factory(params, scene);
	else if(type == "wood") return WoodTexture::factory(params, scene);
	else if(type == "voronoi") return VoronoiTexture::factory(params, scene);
	else if(type == "musgrave") return MusgraveTexture::factory(params, scene);
	else if(type == "distorted_noise") return DistortedNoiseTexture::factory(params, scene);
	else if(type == "rgb_cube") return RgbCubeTexture::factory(params, scene);
	else if(type == "image") return ImageTexture::factory(params, scene);
	else return nullptr;
}

END_YAFARAY
