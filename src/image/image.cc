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
#include "image/image_color_alpha.h"
#include "image/image_color_alpha_optimized.h"
#include "image/image_color_alpha_compressed.h"
#include "image/image_color.h"
#include "image/image_color_optimized.h"
#include "image/image_color_compressed.h"
#include "image/image_gray_alpha.h"
#include "image/image_gray.h"
#include "image/image_gray_optimized.h"
#include "common/file.h"
#include "common/string.h"
#include "format/format.h"
#include "common/logger.h"
#include "common/param.h"

BEGIN_YAFARAY

Image * Image::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	if(logger.isDebug()) logger.logDebug("**Image::factory params");
	int width = 100;
	int height = 100;
	std::string image_type_str = "ColorAlpha";
	std::string image_optimization_str = "optimized";
	std::string color_space_str = "Raw_Manual_Gamma";
	double gamma = 1.0;
	std::string filename;
	params.getParam("type", image_type_str);
	params.getParam("image_optimization", image_optimization_str);
	params.getParam("filename", filename);
	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("color_space", color_space_str);
	params.getParam("gamma", gamma);
	Image::Optimization optimization = Image::getOptimizationTypeFromName(image_optimization_str);
	const Type type = Image::getTypeFromName(image_type_str);
	ColorSpace color_space = Rgb::colorSpaceFromName(color_space_str);

	Image *image = nullptr;

	if(filename.empty())
	{
		logger.logVerbose("Image '", name, "': creating empty image with width=", width, " height=", height);
	}
	else
	{
		const Path path(filename);
		ParamMap format_params;
		format_params["type"] = string::toLower(path.getExtension());
		std::unique_ptr<Format> format = std::unique_ptr<Format>(Format::factory(logger, format_params));
		if(format)
		{
			if(format->isHdr())
			{
				if(color_space != ColorSpace::LinearRgb && logger.isVerbose()) logger.logVerbose("Image: The image is a HDR/EXR file: forcing linear RGB and ignoring selected color space '", color_space_str, "' and the gamma setting.");
				color_space = LinearRgb;
				if(image_optimization_str != "none" && logger.isVerbose()) logger.logVerbose("Image: The image is a HDR/EXR file: forcing texture optimization to 'none' and ignoring selected texture optimization '", image_optimization_str, "'");
				optimization = Image::Optimization::None;
			}
			if(type == Type::Gray || type == Type::GrayAlpha) format->setGrayScaleSetting(true);
			image = format->loadFromFile(filename, optimization, color_space, gamma);
		}

		if(image)
		{
			logger.logInfo("Image '", name, "': loaded from file '", filename, "'");
		}
		else
		{
			logger.logError("Image '", name, "': Couldn't load from file '", filename, "', creating empty image with width=", width, " height=", height);
		}
	}
	if(!image) image = Image::factory(logger, width, height, type, optimization);
	if(image)
	{
		image->color_space_ = color_space;
		image->gamma_ = gamma;
	}
	return image;
}

Image *Image::factory(Logger &logger, int width, int height, const Type &type, const Optimization &optimization)
{
	if(logger.isDebug()) logger.logDebug("**Image::factory");
	if(type == Type::ColorAlpha)
	{
		switch(optimization)
		{
			case Optimization::Optimized: return new ImageColorAlphaOptimized(width, height);
			case Optimization::Compressed: return new ImageColorAlphaCompressed(width, height);
			default: return new ImageColorAlpha(width, height);
		}
	}
	else if(type == Type::Color)
	{
		switch(optimization)
		{
			case Optimization::Optimized: return new ImageColorOptimized(width, height);
			case Optimization::Compressed: return new ImageColorCompressed(width, height);
			default: return new ImageColor(width, height);
		}
	}
	else if(type == Type::GrayAlpha)
	{
		return new ImageGrayAlpha(width, height);
	}
	else if(type == Type::Gray)
	{
		switch(optimization)
		{
			case Optimization::Compressed:
			case Optimization::Optimized: return new ImageGrayOptimized(width, height);
			default: return new ImageGray(width, height);
		}
	}
	else return nullptr;
}

Image::Type Image::imageTypeWithAlpha(Type image_type)
{
	switch(image_type)
	{
		case Type::Gray: return Type::GrayAlpha;
		case Type::Color: return Type::ColorAlpha;
		default: return image_type;
	}
}

Image::Type Image::getTypeFromName(const std::string &image_type_name)
{
	if(image_type_name == "ColorAlpha") return Type::ColorAlpha;
	else if(image_type_name == "Color") return Type::Color;
	else if(image_type_name == "GrayAlpha") return Type::GrayAlpha;
	else if(image_type_name == "Gray") return Type::Gray;
	else return Type::None;
}

int Image::getNumChannels(const Type &image_type)
{
	switch(image_type)
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
	switch(image_type)
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
	switch(image_type)
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
	Type type = grayscale ? Type::Gray : Type::Color;
	if(has_alpha) type = Image::imageTypeWithAlpha(type);
	return type;
}

Image::Optimization Image::getOptimizationTypeFromName(const std::string &optimization_type_name)
{
	//Optimized by default
	if(optimization_type_name == "none") return Image::Optimization::None;
	else if(optimization_type_name == "optimized") return Image::Optimization::Optimized;
	else if(optimization_type_name == "compressed") return Image::Optimization::Compressed;
	else return Image::Optimization::Optimized;
}

std::string Image::getOptimizationName(const Optimization &optimization_type)
{
	//Optimized by default
	switch(optimization_type)
	{
		case Image::Optimization::None: return "none";
		case Image::Optimization::Optimized: return "optimized";
		case Image::Optimization::Compressed: return "compressed";
		default: return "optimized";
	}
}

std::string Image::getTypeNameLong(const Type &image_type)
{
	switch(image_type)
	{
		case Type::ColorAlpha: return "Color + Alpha [4 channels]";
		case Type::Color: return "Color [3 channels]";
		case Type::GrayAlpha: return "Gray + Alpha [2 channels]";
		case Type::Gray: return "Gray [1 channel]";
		default: return "unknown image type [0 channels]";
	}
}

std::string Image::getTypeNameShort(const Type &image_type)
{
	switch(image_type)
	{
		case Type::ColorAlpha: return "ColorAlpha";
		case Type::Color: return "Color";
		case Type::GrayAlpha: return "GrayAlpha";
		case Type::Gray: return "Gray";
		default: return "unknown";
	}
}


END_YAFARAY
