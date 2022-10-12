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

Format::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
}

ParamMap Format::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap Format::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<Format *, ParamError> Format::factory(Logger &logger, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	Format *result = nullptr;
	switch(type.value())
	{
		case Type::Tga: result = new TgaFormat(logger, param_error, param_map); break;
		case Type::Hdr: result = new HdrFormat(logger, param_error, param_map); break;
#ifdef HAVE_OPENEXR
		case Type::Exr: result = new ExrFormat(logger, param_error, param_map); break;
#endif // HAVE_OPENEXR
#ifdef HAVE_JPEG
		case Type::Jpg: result = new JpgFormat(logger, param_error, param_map); break;
#endif // HAVE_JPEG
#ifdef HAVE_PNG
		case Type::Png: result = new PngFormat(logger, param_error, param_map); break;
#endif // HAVE_PNG
#ifdef HAVE_TIFF
		case Type::Tif: result = new TifFormat(logger, param_error, param_map); break;
#endif // HAVE_TIFF
		default: param_error.flags_ = ParamError::Flags::ErrorWhileCreating; break;
	}
	return {result, param_error};
}

std::unique_ptr<Image> Format::loadFromMemory(const uint8_t *data, size_t size, const Image::Optimization &optimization, const ColorSpace &color_space, float gamma)
{
	return nullptr;
}

} //namespace yafaray
