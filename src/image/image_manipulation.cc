/****************************************************************************
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

#include "image/image_manipulation.h"
#ifdef HAVE_FREETYPE
#include "image/image_manipulation_freetype.h"
#endif //HAVE_FREETYPE
#ifdef HAVE_OPENCV
#include "image/image_manipulation_opencv.h"
#endif //HAVE_OPENCV
#include "common/logger.h"

namespace yafaray {

Image *image_manipulation::getDenoisedLdrImage(Logger &logger, const Image *image, const DenoiseParams &denoise_params)
{
#ifdef HAVE_OPENCV
	return image_manipulation_opencv::getDenoisedLdrImage(logger, image, denoise_params);
#else //HAVE_OPENCV
	return nullptr;
#endif //HAVE_OPENCV
}

bool image_manipulation::drawTextInImage(Logger &logger, Image *image, const std::string &text_utf_8, float font_size_factor, const std::string &font_path)
{
#ifdef HAVE_FREETYPE
	return image_manipulation_freetype::drawTextInImage(logger, image, text_utf_8, font_size_factor, font_path);
#else //HAVE_FREETYPE
	return false;
#endif //HAVE_FREETYPE
}

Image *image_manipulation::getComposedImage(Logger &logger, const Image *image_1, const Image *image_2, const Image::Position &position_image_2, int overlay_x, int overlay_y)
{
	if(!image_1 || !image_2) return nullptr;
	const int width_1 = image_1->getWidth();
	const int height_1 = image_1->getHeight();
	const int width_2 = image_2->getWidth();
	const int height_2 = image_2->getHeight();
	int width = width_1;
	int height = height_1;
	switch(position_image_2.value())
	{
		case Image::Position::Bottom:
		case Image::Position::Top: height += height_2; break;
		case Image::Position::Left:
		case Image::Position::Right: width += width_2; break;
		case Image::Position::Overlay: break;
		default: return nullptr;
	}
	Image::Params image_params;
	image_params.width_ = width;
	image_params.height_ = height;
	image_params.type_ = image_1->getType();
	image_params.image_optimization_ = image_1->getOptimization();
	auto result = Image::factory(image_params);

	for(int x = 0; x < width; ++x)
	{
		for(int y = 0; y < height; ++y)
		{
			Rgba color;
			switch(position_image_2.value())
			{
				case Image::Position::Bottom:
					if(y >= height_1 && x < width_2) color = image_2->getColor({{x, y - height_1}});
					else if(y < height_1) color = image_1->getColor({{x, y}});
					break;
				case Image::Position::Left:
					if(x < width_2 && y < height_2) color = image_2->getColor({{x, y}});
					else if(x > width_2) color = image_1->getColor({{x - width_2, y}});
					break;
				case Image::Position::Right:
					if(x >= width_1 && y < height_2) color = image_2->getColor({{x - width_1, y}});
					else if(x < width_1) color = image_1->getColor({{x, y}});
					break;
				case Image::Position::Overlay:
					if(x >= overlay_x && x < overlay_x + width_2 && y >= overlay_y && y < overlay_y + height_2) color = image_2->getColor({{x - overlay_x, y - overlay_y}});
					else color = image_1->getColor({{x, y}});
					break;
				case Image::Position::Top:
				default:
					if(y < height_2 && x < width_2) color = image_2->getColor({{x, y}});
					else if(y >= height_2) color = image_1->getColor({{x, y - height_2}});
					break;
			}
			result->setColor({{x, y}}, color);
		}
	}
	return result;
}
void image_manipulation::generateDebugFacesEdges(ImageLayers &film_image_layers, int xstart, int width, int ystart, int height, bool drawborder, const EdgeToonParams &edge_params, const Buffer2D<Gray> &weights)
{
#ifdef HAVE_OPENCV
	image_manipulation_opencv::generateDebugFacesEdges(film_image_layers, xstart, width, ystart, height, drawborder, edge_params, weights);
#endif //HAVE_OPENCV
}

void image_manipulation::generateToonAndDebugObjectEdges(ImageLayers &film_image_layers, int xstart, int width, int ystart, int height, bool drawborder, const EdgeToonParams &edge_params, const Buffer2D<Gray> &weights)
{
#ifdef HAVE_OPENCV
	image_manipulation_opencv::generateToonAndDebugObjectEdges(film_image_layers, xstart, width, ystart, height, drawborder, edge_params, weights);
#endif //HAVE_OPENCV
}

int image_manipulation::generateMipMaps(Logger &logger, std::vector<std::shared_ptr<Image>> &images)
{
#ifdef HAVE_OPENCV
	const int mipmaps_generated = image_manipulation_opencv::generateMipMaps(logger, images);
	if(logger.isVerbose()) logger.logVerbose("Format: mipmap generation done: ", mipmaps_generated, " mipmaps generated.");
	return mipmaps_generated;
#else
	logger.logWarning("Format: cannot generate mipmaps, YafaRay was not built with OpenCV support which is needed for mipmap processing.");
	return 0;
#endif
}

std::string image_manipulation::printDenoiseParams(const DenoiseParams &denoise_params)
{
#ifdef HAVE_OPENCV	//Denoise only works if YafaRay is built with OpenCV support
	if(!denoise_params.enabled_) return "";
	std::stringstream param_string;
	param_string << "Image file denoise enabled [mix=" << denoise_params.mix_ << ", h(Luminance)=" << denoise_params.hlum_ <<  ", h(Chrominance)=" << denoise_params.hcol_ << "]";
	return param_string.str();
#else
	return "";
#endif
}

void image_manipulation::logWarningsMissingLibraries(Logger &logger)
{
#ifndef HAVE_OPENCV
	logger.logWarning("libYafaRay built without OpenCV support. The following functionality will not work: image output denoise, background IBL blur, object/face edge render layers, toon render layer.");
#endif

#ifndef HAVE_FREETYPE
	logger.logWarning("libYafaRay built without FreeType support. There will not be any text in the renderer parameter badges.");
#endif
}


} //namespace yafaray
