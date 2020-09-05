/****************************************************************************
 *      imagehandler.cc: common code for all imagehandlers
 *      This is part of the libYafaRay package
 *      Copyright (C) 2016  David Bluecame
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
#include "common/renderpasses.h"
#include "common/logging.h"

#ifdef HAVE_OPENCV
#include <opencv2/photo/photo.hpp>
#endif


BEGIN_YAFARAY


ImageBuffer::ImageBuffer(int width, int height, int num_channels, const TextureOptimization &optimization): width_(width), height_(height), num_channels_(num_channels), optimization_(optimization)
{
	switch(optimization)
	{
		case TextureOptimization::None:
			if(num_channels_ == 4) rgba_128_float_img_ = new Rgba2DImage_t(width, height);
			else if(num_channels_ == 3) rgb_96_float_img_ = new Rgb2DImage_t(width, height);
			else if(num_channels_ == 1) gray_32_float_img_ = new Gray2DImage_t(width, height);
			break;

		case TextureOptimization::Optimized:
			if(num_channels_ == 4) rgba_40_optimized_img_ = new RgbaOptimizedImage_t(width, height);
			else if(num_channels_ == 3) rgb_32_optimized_img_ = new RgbOptimizedImage_t(width, height);
			else if(num_channels_ == 1) gray_8_optimized_img_ = new GrayOptimizedImage_t(width, height);
			break;

		case TextureOptimization::Compressed:
			if(num_channels_ == 4) rgba_24_compressed_img_ = new RgbaCompressedImage_t(width, height);
			else if(num_channels_ == 3) rgb_16_compressed_img_ = new RgbCompressedImage_t(width, height);
			else if(num_channels_ == 1) gray_8_optimized_img_ = new GrayOptimizedImage_t(width, height);
			break;

		default: break;
	}
}

ImageBuffer ImageBuffer::getDenoisedLdrBuffer(float h_col, float h_lum, float mix) const
{
	ImageBuffer denoised_buffer = ImageBuffer(width_, height_, num_channels_, optimization_);

#ifdef HAVE_OPENCV
	cv::Mat a(height_, width_, CV_8UC3);
	cv::Mat b(height_, width_, CV_8UC3);
	cv::Mat_<cv::Vec3b> a_vec = a;
	cv::Mat_<cv::Vec3b> b_vec = b;

	for(int y = 0; y < height_; ++y)
	{
		for(int x = 0; x < width_; ++x)
		{
			Rgb color = getColor(x, y);
			color.clampRgb01();

			a_vec(y, x)[0] = (uint8_t)(color.getR() * 255);
			a_vec(y, x)[1] = (uint8_t)(color.getG() * 255);
			a_vec(y, x)[2] = (uint8_t)(color.getB() * 255);
		}
	}

	cv::fastNlMeansDenoisingColored(a, b, h_lum, h_col, 7, 21);

	for(int y = 0; y < height_; ++y)
	{
		for(int x = 0; x < width_; ++x)
		{
			Rgba col;
			col.r_ = (mix * (float)b_vec(y, x)[0] + (1.f - mix) * (float)a_vec(y, x)[0]) / 255.0;
			col.g_ = (mix * (float)b_vec(y, x)[1] + (1.f - mix) * (float)a_vec(y, x)[1]) / 255.0;
			col.b_ = (mix * (float)b_vec(y, x)[2] + (1.f - mix) * (float)a_vec(y, x)[2]) / 255.0;
			col.a_ = getColor(x, y).a_;
			denoised_buffer.setColor(x, y, col);
		}
	}
#else //HAVE_OPENCV
	//FIXME: Useless duplication work when OpenCV is not built in... avoid calling this function in the first place if OpenCV support not built.
	//This is kept here for interface compatibility when OpenCV not built in.
	for(int y = 0; y < height_; ++y)
	{
		for(int x = 0; x < width_; ++x)
		{
			denoised_buffer.setColor(x, y, getColor(x, y));
		}
	}
	Y_WARNING << "ImageHandler: built without OpenCV support, image cannot be de-noised." << YENDL;
#endif //HAVE_OPENCV
	return denoised_buffer;
}

ImageBuffer::~ImageBuffer()
{
	if(rgba_40_optimized_img_) { delete rgba_40_optimized_img_; rgba_40_optimized_img_ = nullptr; }
	if(rgba_24_compressed_img_) { delete rgba_24_compressed_img_; rgba_24_compressed_img_ = nullptr; }
	if(rgba_128_float_img_) { delete rgba_128_float_img_; rgba_128_float_img_ = nullptr; }
	if(rgb_32_optimized_img_) { delete rgb_32_optimized_img_; rgb_32_optimized_img_ = nullptr; }
	if(rgb_16_compressed_img_) { delete rgb_16_compressed_img_; rgb_16_compressed_img_ = nullptr; }
	if(rgb_96_float_img_) { delete rgb_96_float_img_; rgb_96_float_img_ = nullptr; }
	if(gray_32_float_img_) { delete gray_32_float_img_; gray_32_float_img_ = nullptr; }
	if(gray_8_optimized_img_) { delete gray_8_optimized_img_; gray_8_optimized_img_ = nullptr; }
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
	if(img_buffer_.empty()) return;

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
			Rgba color = img_buffer_.at(img_index)->getColor(i, j);

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
		img_buffer_.push_back(new ImageBuffer(w_2, h_2, img_buffer_.at(img_index - 1)->getNumChannels(), getTextureOptimization()));

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

				img_buffer_.at(img_index)->setColor(i, j, tmp_col);
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
	img_buffer_.at(img_index)->setColor(x, y, rgba);
}

Rgba ImageHandler::getPixel(int x, int y, int img_index)
{
	return img_buffer_.at(img_index)->getColor(x, y);
}


void ImageHandler::initForOutput(int width, int height, const RenderPasses *render_passes, bool denoise_enabled, int denoise_h_lum, int denoise_h_col, float denoise_mix, bool with_alpha, bool multi_layer, bool grayscale)
{
	has_alpha_ = with_alpha;
	multi_layer_ = multi_layer;
	denoise_ = denoise_enabled;
	denoise_hlum_ = denoise_h_lum;
	denoise_hcol_ = denoise_h_col;
	denoise_mix_ = denoise_mix;
	grayscale_ = grayscale;

	int n_channels = 3;
	if(grayscale_) n_channels = 1;
	else if(has_alpha_) n_channels = 4;

	for(int idx = 0; idx < render_passes->extPassesSize(); ++idx)
	{
		img_buffer_.push_back(new ImageBuffer(width, height, n_channels, TextureOptimization::None));
	}
}

void ImageHandler::clearImgBuffers()
{
	if(!img_buffer_.empty())
	{
		for(size_t idx = 0; idx < img_buffer_.size(); ++idx)
		{
			delete img_buffer_.at(idx);
			img_buffer_.at(idx) = nullptr;
		}
	}
}

END_YAFARAY
