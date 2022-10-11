/****************************************************************************
 *
 *      This is part of the libYafaRay package
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

#include "image/image.h"
#include "image/image_buffer.h"
#include "common/file.h"
#include "common/string.h"
#include "format/format.h"
#include "common/logger.h"
#include "param/param.h"

namespace yafaray {

Image::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(type_);
	PARAM_LOAD(filename_);
	PARAM_ENUM_LOAD(color_space_);
	PARAM_LOAD(gamma_);
	PARAM_ENUM_LOAD(image_optimization_);
	PARAM_LOAD(width_);
	PARAM_LOAD(height_);
}

ParamMap Image::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_ENUM_SAVE(type_);
	PARAM_SAVE(filename_);
	PARAM_ENUM_SAVE(color_space_);
	PARAM_SAVE(gamma_);
	PARAM_ENUM_SAVE(image_optimization_);
	PARAM_SAVE(width_);
	PARAM_SAVE(height_);
	PARAM_SAVE_END;
}

ParamMap Image::getAsParamMap(bool only_non_default) const
{
	return params_.getAsParamMap(only_non_default);
}

Image * Image::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + "::factory 'raw' ParamMap\n" + param_map.logContents());
	auto param_error{Params::meta_.check(param_map, {}, {})};
	std::string type_str;
	param_map.getParam(Params::type_meta_, type_str);
	const Type type{type_str};
	Params params{param_error, param_map};
	Image *image = nullptr;
	if(params.filename_.empty())
	{
		logger.logVerbose("Image '", name, "': creating empty image with width=", params.width_, " height=", params.height_);
	}
	else
	{
		const Path path(params.filename_);
		ParamMap format_params;
		format_params["type"] = string::toLower(path.getExtension());
		std::unique_ptr<Format> format = std::unique_ptr<Format>(Format::factory(logger, format_params).first);
		if(format)
		{
			if(format->isHdr())
			{
				if(params.color_space_ != ColorSpace::LinearRgb && logger.isVerbose()) logger.logVerbose("Image: The image is a HDR/EXR file: forcing linear RGB and ignoring selected color space '", params.color_space_.print(), "' and the gamma setting.");
				params.color_space_ = ColorSpace::LinearRgb;
				if(params.image_optimization_ != Optimization::None && logger.isVerbose()) logger.logVerbose("Image: The image is a HDR/EXR file: forcing texture optimization to 'none' and ignoring selected texture optimization '", params.image_optimization_.print(), "'");
				params.image_optimization_ = Image::Optimization::None;
			}
			if(type == Type::Gray || type == Type::GrayAlpha) format->setGrayScaleSetting(true);
			image = format->loadFromFile(params.filename_, params.image_optimization_, params.color_space_, params.gamma_);
		}

		if(image)
		{
			logger.logInfo("Image '", name, "': loaded from file '", params.filename_, "'");
		}
		else
		{
			logger.logError("Image '", name, "': Couldn't load from file '", params.filename_, "', creating empty image with width=", params.width_, " height=", params.height_);
		}
	}
	if(!image) image = Image::factory(params);
	if(param_error.notOk()) logger.logWarning(param_error.print<Image>(name, {}));
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + image->params_.getAsParamMap(true).print());
	return image;
}

Image *Image::factory(const Params &params)
{
	switch(params.type_.value())
	{
		case Type::Gray:
			switch(params.image_optimization_.value())
			{
				case Optimization::Compressed:
				case Optimization::Optimized:
					return new ImageBuffer<Gray8>(params); //!< Optimized grayscale (8 bit/pixel) image buffer
				default:
					return new ImageBuffer<Gray>(params); //!< Grayscale float (32 bit/pixel) image buffer
			}

		case Type::GrayAlpha:
			return new ImageBuffer<GrayAlpha>(params); //!< Grayscale with alpha float (64 bit/pixel) image buffer

		case Type::Color:
			switch(params.image_optimization_.value())
			{
				case Optimization::Optimized:
					return new ImageBuffer<Rgb101010>(params); //!< Optimized Rgb (32 bit/pixel) image buffer
				case Optimization::Compressed:
					return new ImageBuffer<Rgb565>(params); //!< Compressed Rgb (16 bit/pixel) [LOSSY!] image buffer
				default:
					return new ImageBuffer<Rgb>(params); //!< Rgb float image buffer (96 bit/pixel)
			}

		case Type::ColorAlpha:
		default:
			switch(params.image_optimization_.value())
			{
				case Optimization::Optimized:
					return new ImageBuffer<Rgba1010108>(params); //!< Optimized Rgba (40 bit/pixel) image buffer
				case Optimization::Compressed:
					return new ImageBuffer<Rgba7773>(params); //!< Compressed Rgba (24 bit/pixel) [LOSSY!] image buffer
				default:
					return new ImageBuffer<RgbAlpha>(params); //!< Rgba float image buffer (128 bit/pixel)
			}
	}
}

Image::Type Image::imageTypeWithAlpha(Type image_type)
{
	switch(image_type.value())
	{
		case Type::Gray: return Type{Type::GrayAlpha};
		case Type::Color: return Type{Type::ColorAlpha};
		default: return image_type;
	}
}

Image::Type Image::getTypeFromName(const std::string &image_type_name)
{
	return Type{image_type_name};
}

int Image::getNumChannels(const Type &image_type)
{
	switch(image_type.value())
	{
		case Type::ColorAlpha: return 4;
		case Type::Color: return 3;
		case Type::GrayAlpha: return 2;
		case Type::Gray: return 1;
		default: return 0;
	}
}

bool Image::hasAlpha(const Type &image_type)
{
	switch(image_type.value())
	{
		case Type::Color:
		case Type::Gray: return false;
		case Type::ColorAlpha:
		case Type::GrayAlpha:
		default: return true;
	}
}

bool Image::isGrayscale(const Type &image_type)
{
	switch(image_type.value())
	{
		case Type::Gray:
		case Type::GrayAlpha: return true;
		case Type::Color:
		case Type::ColorAlpha:
		default: return false;
	}
}

Image::Type Image::getTypeFromSettings(bool has_alpha, bool grayscale)
{
	Type type{grayscale ? Type::Gray : Type::Color};
	if(has_alpha) type = Image::imageTypeWithAlpha(type);
	return type;
}

Image::Optimization Image::getOptimizationTypeFromName(const std::string &optimization_type_name)
{
	return Image::Optimization{optimization_type_name};
}

std::string Image::getOptimizationName(const Optimization &optimization_type)
{
	return optimization_type.print();
}

std::string Image::getTypeNameLong(const Type &image_type)
{
	return image_type.printDescription();
}

std::string Image::getTypeNameShort(const Type &image_type)
{
	return image_type.print();
}


} //namespace yafaray
