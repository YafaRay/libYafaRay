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

std::map<std::string, const ParamMeta *> Image::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(type_);
	PARAM_META(filename_);
	PARAM_META(color_space_);
	PARAM_META(gamma_);
	PARAM_META(image_optimization_);
	PARAM_META(width_);
	PARAM_META(height_);
	return param_meta_map;
}

Image::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_ENUM_LOAD(type_);
	PARAM_LOAD(filename_);
	PARAM_ENUM_LOAD(color_space_);
	PARAM_LOAD(gamma_);
	PARAM_ENUM_LOAD(image_optimization_);
	PARAM_LOAD(width_);
	PARAM_LOAD(height_);
}

ParamMap Image::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_ENUM_SAVE(type_);
	PARAM_SAVE(filename_);
	PARAM_ENUM_SAVE(color_space_);
	PARAM_SAVE(gamma_);
	PARAM_ENUM_SAVE(image_optimization_);
	PARAM_SAVE(width_);
	PARAM_SAVE(height_);
	return param_map;
}

std::pair<std::unique_ptr<Image>, ParamResult> Image::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + "::factory 'raw' ParamMap\n" + param_map.logContents());
	auto param_result{class_meta::check<Params>(param_map, {}, {})};
	std::string type_str;
	param_map.getParam(Params::type_meta_, type_str);
	const Type type{type_str};
	Params params{param_result, param_map};
	std::unique_ptr<Image> image;
	if(params.filename_.empty())
	{
		logger.logVerbose("Image '", name, "': creating empty image with width=", params.width_, " height=", params.height_);
	}
	else
	{
		const Path path(params.filename_);
		ParamMap format_params;
		format_params["type"] = string::toLower(path.getExtension());
		auto format{Format::factory(logger, format_params).first};
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
	if(param_result.notOk()) logger.logWarning(param_result.print<Image>(name, {}));
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + image->getAsParamMap(true).print());
	image->setName(name);
	return {std::move(image), param_result};
}

std::unique_ptr<Image> Image::factory(const Params &params)
{
	switch(params.type_.value())
	{
		case Type::Gray:
			switch(params.image_optimization_.value())
			{
				case Optimization::Compressed:
				case Optimization::Optimized:
					return std::make_unique<ImageBuffer<Gray8>>(params); //!< Optimized grayscale (8 bit/pixel) image buffer
				default:
					return std::make_unique<ImageBuffer<Gray>>(params); //!< Grayscale float (32 bit/pixel) image buffer
			}

		case Type::GrayAlpha:
			return std::make_unique<ImageBuffer<GrayAlpha>>(params); //!< Grayscale with alpha float (64 bit/pixel) image buffer

		case Type::Color:
			switch(params.image_optimization_.value())
			{
				case Optimization::Optimized:
					return std::make_unique<ImageBuffer<Rgb101010>>(params); //!< Optimized Rgb (32 bit/pixel) image buffer
				case Optimization::Compressed:
					return std::make_unique<ImageBuffer<Rgb565>>(params); //!< Compressed Rgb (16 bit/pixel) [LOSSY!] image buffer
				default:
					return std::make_unique<ImageBuffer<Rgb>>(params); //!< Rgb float image buffer (96 bit/pixel)
			}

		case Type::ColorAlpha:
		default:
			switch(params.image_optimization_.value())
			{
				case Optimization::Optimized:
					return std::make_unique<ImageBuffer<Rgba1010108>>(params); //!< Optimized Rgba (40 bit/pixel) image buffer
				case Optimization::Compressed:
					return std::make_unique<ImageBuffer<Rgba7773>>(params); //!< Compressed Rgba (24 bit/pixel) [LOSSY!] image buffer
				default:
					return std::make_unique<ImageBuffer<RgbAlpha>>(params); //!< Rgba float image buffer (128 bit/pixel)
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

std::string Image::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<image name=\"" << getName() << "\">" << std::endl;
	ss << param_map.exportMap(indent_level + 1, container_export_type, only_export_non_default_parameters, params_.getParamMetaMap(), {});
	ss << std::string(indent_level, '\t') << "</image>" << std::endl;
	return ss.str();
}


} //namespace yafaray
