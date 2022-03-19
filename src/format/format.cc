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
#include "format/format_tga.h"
#include "format/format_hdr.h"
#ifdef HAVE_OPENEXR
#include "format/format_exr.h"
#endif
#ifdef HAVE_JPEG
#include "format/format_jpg.h"
#endif
#ifdef HAVE_PNG
#include "format/format_png.h"
#endif
#ifdef HAVE_TIFF
#include "format/format_tif.h"
#endif
#include "common/param.h"
#include "common/logger.h"

BEGIN_YAFARAY

Format * Format::factory(Logger &logger, ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Format");
		params.logContents(logger);
	}

	std::string type;
	params.getParam("type", type);
	type = string::toLower(type);

	if(type == "tga" || type == "tpic") return new TgaFormat(logger);
	else if(type == "hdr" || type == "pic") return new HdrFormat(logger);
#ifdef HAVE_OPENEXR
	else if(type == "exr") return new ExrFormat(logger);
#endif // HAVE_OPENEXR
#ifdef HAVE_JPEG
	else if(type == "jpg" || type == "jpeg") return new JpgFormat(logger);
#endif // HAVE_JPEG
#ifdef HAVE_PNG
	else if(type == "png") return new PngFormat(logger);
#endif // HAVE_PNG
#ifdef HAVE_TIFF
	else if(type == "tif" || type == "tiff") return new TifFormat(logger);
#endif // HAVE_TIFF
	else
	{
		std::string name;
		params.getParam("name", name);
		logger.logError("Cannot process file, libYafaRay has not been built with support for image file format '" + type + "'");
		return nullptr;
	}
}

Image * Format::loadFromMemory(const uint8_t *data, size_t size, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	return nullptr;
}

END_YAFARAY
