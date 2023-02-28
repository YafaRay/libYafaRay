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
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

//Type map definitions conditionally defined. These defines are only available in this file and not in the header, so they *must* be defined here.
const EnumMap<unsigned char> Format::Type::map_{{
	{"tga", Tga, "TGA format"},
	{"tpic", Tga, "TGA format, alternative file extension"},
	{"hdr", Hdr, "HDR format"},
	{"pic", Hdr, "HDR format, alternative file extension"},
#ifdef HAVE_OPENEXR
	{"exr", Exr, "EXR format"},
#endif // HAVE_OPENEXR
#ifdef HAVE_JPEG
	{"jpg", Jpg, "JPEG format"},
	{"jpeg", Jpg, "JPEG format, alternative file extension"},
#endif // HAVE_JPEG
#ifdef HAVE_PNG
	{"png", Png, "PNG format"},
#endif // HAVE_PNG
#ifdef HAVE_TIFF
	{"tif", Tif, "TIFF format"},
	{"tiff", Tif, "TIFF format, alternative file extension"},
#endif // HAVE_TIFF
}};

std::map<std::string, const ParamMeta *> Format::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	return param_meta_map;
}

Format::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
}

ParamMap Format::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	return param_map;
}

std::pair<std::unique_ptr<Format>, ParamResult> Format::factory(Logger &logger, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {})};
	std::unique_ptr<Format> format;
	switch(type.value())
	{
		case Type::Tga: format = std::make_unique<TgaFormat>(logger, param_result, param_map); break;
		case Type::Hdr: format = std::make_unique<HdrFormat>(logger, param_result, param_map); break;
#ifdef HAVE_OPENEXR
		case Type::Exr: format = std::make_unique<ExrFormat>(logger, param_result, param_map); break;
#endif // HAVE_OPENEXR
#ifdef HAVE_JPEG
		case Type::Jpg: format = std::make_unique<JpgFormat>(logger, param_result, param_map); break;
#endif // HAVE_JPEG
#ifdef HAVE_PNG
		case Type::Png: format = std::make_unique<PngFormat>(logger, param_result, param_map); break;
#endif // HAVE_PNG
#ifdef HAVE_TIFF
		case Type::Tif: format = std::make_unique<TifFormat>(logger, param_result, param_map); break;
#endif // HAVE_TIFF
		default: param_result.flags_ = YAFARAY_RESULT_ERROR_WHILE_CREATING; break;
	}
	return {std::move(format), param_result};
}

std::unique_ptr<Image> Format::loadFromMemory(const uint8_t *data, size_t size, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	return nullptr;
}

} //namespace yafaray
