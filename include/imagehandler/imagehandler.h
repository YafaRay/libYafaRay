#pragma once
/****************************************************************************
 *
 *      imagehandler.h: image load and save abstract class
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
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
 *
 */

#ifndef YAFARAY_IMAGEHANDLER_H
#define YAFARAY_IMAGEHANDLER_H

#include "constants.h"
#include "image/image.h"

BEGIN_YAFARAY

class PassesSettings;
class ParamMap;
class Scene;

enum class InterpolationType : int { None, Bilinear, Bicubic, Trilinear, Ewa };

class MipMapParams final
{
	public:
		MipMapParams(float force_image_level): force_image_level_(force_image_level) {}
		MipMapParams(float dsdx, float dtdx, float dsdy, float dtdy): ds_dx_(dsdx), dt_dx_(dtdx), ds_dy_(dsdy), dt_dy_(dtdy) {}

		float force_image_level_ = 0.f;
		float ds_dx_ = 0.f;
		float dt_dx_ = 0.f;
		float ds_dy_ = 0.f;
		float dt_dy_ = 0.f;
};

class LIBYAFARAY_EXPORT ImageHandler
{
	public:
		static ImageHandler *factory(ParamMap &params, Scene &scene);
		virtual ~ImageHandler() = default;
		virtual bool loadFromFile(const std::string &name) = 0;
		virtual bool loadFromMemory(const uint8_t *data, size_t size) {return false; }
		virtual bool saveToFile(const std::string &name, int img_index = 0) = 0;
		virtual bool saveToFileMultiChannel(const std::string &name, const PassesSettings *passes_settings) { return false; };
		virtual bool isHdr() const { return false; }
		bool isMultiLayer() const { return multi_layer_; }
		bool denoiseEnabled() const { return denoise_; }
		Image::Optimization getImageOptimization() const { return image_optimization_; }
		void setImageOptimization(const Image::Optimization &image_optimization) { image_optimization_ = image_optimization; }
		void setGrayScaleSetting(bool grayscale) { grayscale_ = grayscale; }
		int getWidth(int img_index = 0) const { return images_.at(img_index).getWidth(); }
		int getHeight(int img_index = 0) const { return images_.at(img_index).getHeight(); }
		std::string getDenoiseParams() const;
		void generateMipMaps();
		int getHighestImgIndex() const { return (int) images_.size() - 1; }
		void setColorSpace(ColorSpace color_space, float gamma) { color_space_ = color_space; gamma_ = gamma; }
		void putPixel(int x, int y, const Rgba &rgba, int img_index = 0);
		Rgba getPixel(int x, int y, int img_index = 0);
		void initForOutput(int width, int height, const PassesSettings *passes_settings, bool denoise_enabled, int denoise_h_lum, int denoise_h_col, float denoise_mix, bool with_alpha = false, bool multi_layer = false, bool grayscale = false);

	protected:
		std::string handler_name_;
		int width_ = 0;
		int height_ = 0;
		bool has_alpha_ = false;
		bool grayscale_ = false;	//!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case.
		Image::Optimization image_optimization_ = Image::Optimization::Optimized;
		ColorSpace color_space_ = RawManualGamma;
		float gamma_ = 1.f;
		std::vector<Image> images_;
		bool multi_layer_ = false;
		bool denoise_ = false;
		int denoise_hlum_ = 3;
		int denoise_hcol_ = 3;
		float denoise_mix_ = 0.8f;	//!< Mix factor between the de-noised image and the original "noisy" image to avoid banding artifacts in images with all noise removed.
};

END_YAFARAY

#endif // YAFARAY_IMAGE_H
