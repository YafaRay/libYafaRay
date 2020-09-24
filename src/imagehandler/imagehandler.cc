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
#include "common/param.h"
#include "common/logging.h"
#include "render/passes.h"

#ifdef HAVE_OPENCV
#include <opencv2/photo/photo.hpp>
#endif

BEGIN_YAFARAY

ImageHandler *ImageHandler::factory(ParamMap &params, Scene &scene)
{
	std::string type;
	params.getParam("type", type);

	type = toLower__(type);
	if(type == "tiff") type = "tif";
	else if(type == "tpic") type = "tga";
	else if(type == "jpeg") type = "jpg";
	else if(type == "pic") type = "hdr";


	if(type == "tga") return TgaHandler::factory(params, scene);
	else if(type == "hdr") return HdrHandler::factory(params, scene);
#ifdef HAVE_OPENEXR
	else if(type == "exr") return ExrHandler::factory(params, scene);
#endif // HAVE_OPENEXR
#ifdef HAVE_JPEG
	else if(type == "jpg") return JpgHandler::factory(params, scene);
#endif // HAVE_JPEG
#ifdef HAVE_PNG
	else if(type == "png") return PngHandler::factory(params, scene);
#endif // HAVE_PNG
#ifdef HAVE_TIFF
	else if(type == "tif") return TifHandler::factory(params, scene);
#endif // HAVE_TIFF
	else return nullptr;
}


std::string ImageHandler::getDenoiseParams() const
{
#ifdef HAVE_OPENCV	//Denoise only works if YafaRay is built with OpenCV support
	if(!denoise_) return "";
	std::stringstream param_string;
	param_string << "| Image file denoise enabled [mix=" << denoise_mix_ << ", h(Luminance)=" << denoise_hlum_ << ", h(Chrominance)=" << denoise_hcol_ << "]" << YENDL;
	return param_string.str();
#else
	return "";
#endif
}


void ImageHandler::generateMipMaps()
{
	if(images_.empty()) return;

#ifdef HAVE_OPENCV
	int img_index = 0;
	//bool blur_seamless = true;
	int w = width_, h = height_;

	Y_VERBOSE << "ImageHandler: generating mipmaps for texture of resolution [" << w << " x " << h << "]" << YENDL;

	cv::Mat a(h, w, CV_32FC4);
	cv::Mat_<cv::Vec4f> a_vec = a;

	for(int j = 0; j < h; ++j)
	{
		for(int i = 0; i < w; ++i)
		{
			Rgba color = images_[img_index].getColor(i, j);

			a_vec(j, i)[0] = color.getR();
			a_vec(j, i)[1] = color.getG();
			a_vec(j, i)[2] = color.getB();
			a_vec(j, i)[3] = color.getA();
		}
	}

	//Mipmap generation using the temporary full float buffer to reduce information loss
	while(w > 1 || h > 1)
	{
		int w_2 = (w + 1) / 2;
		int h_2 = (h + 1) / 2;
		++img_index;
		images_.emplace_back(Image{w_2, h_2, images_[img_index - 1].getType(), getImageOptimization()});

		cv::Mat b(h_2, w_2, CV_32FC4);
		cv::Mat_<cv::Vec4f> b_vec = b;
		cv::resize(a, b, cv::Size(w_2, h_2), 0, 0, cv::INTER_AREA);
		//A = B;

		for(int j = 0; j < h_2; ++j)
		{
			for(int i = 0; i < w_2; ++i)
			{
				Rgba tmp_col(0.f);
				tmp_col.r_ = b_vec(j, i)[0];
				tmp_col.g_ = b_vec(j, i)[1];
				tmp_col.b_ = b_vec(j, i)[2];
				tmp_col.a_ = b_vec(j, i)[3];

				images_[img_index].setColor(i, j, tmp_col);
			}
		}

		w = w_2;
		h = h_2;
		Y_DEBUG << "ImageHandler: generated mipmap " << img_index << " [" << w_2 << " x " << h_2 << "]" << YENDL;
	}

	Y_VERBOSE << "ImageHandler: mipmap generation done: " << img_index << " mipmaps generated." << YENDL;
#else
	Y_WARNING << "ImageHandler: cannot generate mipmaps, YafaRay was not built with OpenCV support which is needed for mipmap processing." << YENDL;
#endif
}


void ImageHandler::putPixel(int x, int y, const Rgba &rgba, int img_index)
{
	images_[img_index].setColor(x, y, rgba);
}

Rgba ImageHandler::getPixel(int x, int y, int img_index)
{
	return images_[img_index].getColor(x, y);
}


void ImageHandler::initForOutput(int width, int height, const PassesSettings *passes_settings, bool denoise_enabled, int denoise_h_lum, int denoise_h_col, float denoise_mix, bool with_alpha, bool multi_layer, bool grayscale)
{
	has_alpha_ = with_alpha;
	multi_layer_ = multi_layer;
	denoise_ = denoise_enabled;
	denoise_hlum_ = denoise_h_lum;
	denoise_hcol_ = denoise_h_col;
	denoise_mix_ = denoise_mix;
	grayscale_ = grayscale;

	ImageType type = ImageType::Color;
	if(grayscale_) type = ImageType::Gray;
	else if(has_alpha_) type = ImageType::ColorAlpha;

	for(size_t idx = 0; idx < passes_settings->extPassesSettings().size(); ++idx)
	{
		images_.emplace_back(Image{width, height, type, Image::Optimization::None});
	}
}

END_YAFARAY
