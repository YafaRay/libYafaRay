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

#include "format/format.h"
#include "format/format_exr.h"
#include "format/format_hdr.h"
#include "format/format_jpg.h"
#include "format/format_png.h"
#include "format/format_tga.h"
#include "format/format_tif.h"
#include "common/param.h"
#include "common/logger.h"

BEGIN_YAFARAY

std::unique_ptr<Format> Format::factory(Logger &logger, ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Format");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);

	type = toLower_global(type);
	if(type == "tiff") type = "tif";
	else if(type == "tpic") type = "tga";
	else if(type == "jpeg") type = "jpg";
	else if(type == "pic") type = "hdr";

	if(type == "tga") return TgaFormat::factory(logger, params);
	else if(type == "hdr") return HdrFormat::factory(logger, params);
#ifdef HAVE_OPENEXR
	else if(type == "exr") return ExrFormat::factory(logger, params);
#endif // HAVE_OPENEXR
#ifdef HAVE_JPEG
	else if(type == "jpg") return JpgFormat::factory(logger, params);
#endif // HAVE_JPEG
#ifdef HAVE_PNG
	else if(type == "png") return PngFormat::factory(logger, params);
#endif // HAVE_PNG
#ifdef HAVE_TIFF
	else if(type == "tif") return TifFormat::factory(logger, params);
#endif // HAVE_TIFF
	else return nullptr;
}

std::unique_ptr<Image> Format::loadFromMemory(const uint8_t *data, size_t size, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	return nullptr;
}

END_YAFARAY
