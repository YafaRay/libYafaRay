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

#include "imagehandler/imagehandler.h"
#include "imagehandler/imagehandler_exr.h"
#include "imagehandler/imagehandler_hdr.h"
#include "imagehandler/imagehandler_jpg.h"
#include "imagehandler/imagehandler_png.h"
#include "imagehandler/imagehandler_tga.h"
#include "imagehandler/imagehandler_tif.h"
#include "common/environment.h"
#include "common/param.h"

BEGIN_YAFARAY

ImageHandler *ImageHandler::factory(ParamMap &params, RenderEnvironment &render)
{
	std::string type;
	params.getParam("type", type);

	type = toLower__(type);
	if(type == "tiff") type = "tif";
	else if(type == "tpic") type = "tga";
	else if(type == "jpeg") type = "jpg";
	else if(type == "pic") type = "hdr";


	if(type == "exr") return ExrHandler::factory(params, render);
	else if(type == "hdr") return HdrHandler::factory(params, render);
	else if(type == "jpg") return JpgHandler::factory(params, render);
	else if(type == "png") return PngHandler::factory(params, render);
	else if(type == "tga") return TgaHandler::factory(params, render);
	else if(type == "tif") return TifHandler::factory(params, render);
	else return nullptr;
}

END_YAFARAY
