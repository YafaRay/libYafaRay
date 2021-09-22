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

#ifdef HAVE_OPENCV
#include <opencv2/photo/photo.hpp>
#endif

BEGIN_YAFARAY

std::unique_ptr<Image> Image::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	if(logger.isDebug()) logger.logDebug("**Image::factory params");
	int width = 100;
	int height = 100;
	std::string name;
	std::string image_type_str = "ColorAlpha";
	std::string image_optimization_str = "optimized";
	std::string color_space_str = "Raw_Manual_Gamma";
	double gamma = 1.0;
	std::string filename;
	params.getParam("name", name);
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

	std::unique_ptr<Image> image;

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

std::unique_ptr<Image> Image::factory(Logger &logger, int width, int height, const Type &type, const Optimization &optimization)
{
	if(logger.isDebug()) logger.logDebug("**Image::factory");
	return std::unique_ptr<Image>(Image::factoryRawPointer(logger, width, height, type, optimization));
}

Image *Image::factoryRawPointer(Logger &logger, int width, int height, const Type &type, const Optimization &optimization)
{
	if(logger.isDebug()) logger.logDebug("**Image::factoryRawPointer");
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

std::unique_ptr<Image> Image::getDenoisedLdrImage(Logger &logger, const Image *image, const DenoiseParams &denoise_params)
{
#ifdef HAVE_OPENCV
	if(!denoise_params.enabled_) return nullptr;

	std::unique_ptr<Image> image_denoised = Image::factory(logger, image->getWidth(), image->getHeight(), image->getType(), image->getOptimization());
	if(!image_denoised) return image_denoised;

	const int width = image_denoised->getWidth();
	const int height = image_denoised->getHeight();

	cv::Mat a(height, width, CV_8UC3);
	cv::Mat b(height, width, CV_8UC3);
	cv::Mat_<cv::Vec3b> a_vec = a;
	cv::Mat_<cv::Vec3b> b_vec = b;

	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x)
		{
			Rgb color = image->getColor(x, y);
			color.clampRgb01();

			a_vec(y, x)[0] = (uint8_t)(color.getR() * 255);
			a_vec(y, x)[1] = (uint8_t)(color.getG() * 255);
			a_vec(y, x)[2] = (uint8_t)(color.getB() * 255);
		}
	}

	cv::fastNlMeansDenoisingColored(a, b, denoise_params.hlum_, denoise_params.hcol_, 7, 21);

	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x)
		{
			Rgba col;
			col.r_ = (denoise_params.mix_ * (float)b_vec(y, x)[0] + (1.f - denoise_params.mix_) * (float)a_vec(y, x)[0]) / 255.0;
			col.g_ = (denoise_params.mix_ * (float)b_vec(y, x)[1] + (1.f - denoise_params.mix_) * (float)a_vec(y, x)[1]) / 255.0;
			col.b_ = (denoise_params.mix_ * (float)b_vec(y, x)[2] + (1.f - denoise_params.mix_) * (float)a_vec(y, x)[2]) / 255.0;
			col.a_ = image->getColor(x, y).a_;
			image_denoised->setColor(x, y, col);
		}
	}
	return image_denoised;
#else //HAVE_OPENCV
	return nullptr;
#endif //HAVE_OPENCV
}

std::unique_ptr<Image> Image::getComposedImage(Logger &logger, const Image *image_1, const Image *image_2, const Position &position_image_2, int overlay_x, int overlay_y)
{
	if(!image_1 || !image_2) return nullptr;
	const int width_1 = image_1->getWidth();
	const int height_1 = image_1->getHeight();
	const int width_2 = image_2->getWidth();
	const int height_2 = image_2->getHeight();
	int width = width_1;
	int height = height_1;
	switch(position_image_2)
	{
		case Position::Bottom:
		case Position::Top: height += height_2; break;
		case Position::Left:
		case Position::Right: width += width_2; break;
		case Position::Overlay: break;
		default: return nullptr;
	}
	std::unique_ptr<Image> result = Image::factory(logger, width, height, image_1->getType(), image_1->getOptimization());

	for(int x = 0; x < width; ++x)
	{
		for(int y = 0; y < height; ++y)
		{
			Rgba color;
			if(position_image_2 == Position::Top)
			{
				if(y < height_2 && x < width_2) color = image_2->getColor(x, y);
				else if(y >= height_2) color = image_1->getColor(x, y - height_2);
			}
			else if(position_image_2 == Position::Bottom)
			{
				if(y >= height_1 && x < width_2) color = image_2->getColor(x, y - height_1);
				else if(y < height_1) color = image_1->getColor(x, y);
			}
			else if(position_image_2 == Position::Left)
			{
				if(x < width_2 && y < height_2) color = image_2->getColor(x, y);
				else if(x > width_2) color = image_1->getColor(x - width_2, y);
			}
			else if(position_image_2 == Position::Right)
			{
				if(x >= width_1 && y < height_2) color = image_2->getColor(x - width_1, y);
				else if(x < width_1) color = image_1->getColor(x, y);
			}
			else if(position_image_2 == Position::Overlay)
			{
				if(x >= overlay_x && x < overlay_x + width_2 && y >= overlay_y && y < overlay_y + height_2) color = image_2->getColor(x - overlay_x, y - overlay_y);
				else color = image_1->getColor(x, y);
			}
			result->setColor(x, y, color);
		}
	}
	return result;
}

END_YAFARAY
