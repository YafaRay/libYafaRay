#pragma once
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
 *
 */

#ifndef YAFARAY_IMAGE_H
#define YAFARAY_IMAGE_H

#include "constants.h"
#include <string>

BEGIN_YAFARAY

class Rgba;
class Layer;

struct DenoiseParams
{
	bool enabled_ = false;
	int hlum_ = 3;
	int hcol_ = 3;
	float mix_ = 0.8f;	//!< Mix factor between the de-noised image and the original "noisy" image to avoid banding artifacts in images with all noise removed.
};

class Image final
{
	public:
		enum class Type : int { None, Gray, GrayAlpha, GrayWeight, GrayAlphaWeight, Color, ColorAlpha, ColorAlphaWeight };
		enum class Optimization : int { None, Optimized, Compressed };
		enum class Position : int { None, Top, Bottom, Left, Right, Overlay };
		Image(int width, int height, const Type &type, const Optimization &optimization);
		Image() = default;
		Image(const Image &image);
		Image(Image &&image);
		~Image();

		int getWidth() const { return width_; }
		int getHeight() const { return height_; }
		Type getType() const { return type_; }
		std::string getTypeName() const { return getTypeName(type_); }
		int getNumChannels() const { return getNumChannels(type_); }
		Optimization getOptimization() const { return optimization_; }
		Rgba getColor(int x, int y) const;
		float getFloat(int x, int y) const;
		float getWeight(int x, int y) const;
		bool hasAlpha() const;
		bool isGrayscale() const;
		int getInt(int x, int y) const;
		void setColor(int x, int y, const Rgba &col);
		void setFloat(int x, int y, float val);
		void setWeight(int x, int y, float val);
		void setInt(int x, int y, int val);
		void clear();

		static Type imageTypeWithAlpha(Type image_type);
		static Type imageTypeWithWeight(Type image_type);
		static std::string getTypeName(const Type &image_type);
		static Type getTypeFromName(const std::string &image_type_name);
		static int getNumChannels(const Type &image_type);
		static Optimization getOptimizationTypeFromName(const std::string &optimization_type_name);
		static std::string getOptimizationName(const Optimization &optimization_type);
		static bool hasAlpha(const Type &image_type);
		static bool isGrayscale(const Type &image_type);
		static Type getTypeFromSettings(bool has_alpha, bool grayscale, bool has_weight = false);
		static const Image *getDenoisedLdrImage(const Image *image, const DenoiseParams &denoise_params); //!< Provides a denoised buffer, but only works with LDR images (that can be represented in 8-bit 0..255 values). If attempted with HDR images they would lose the HDR range and become unusable!
		static Image *getComposedImage(const Image *image_1, const Image *image_2, const Position &position_image_2, int overlay_x = 0, int overlay_y = 0);

	protected:
		int width_;
		int height_;
		Type type_;
		Optimization optimization_;
		void *buffer_ = nullptr;
};

END_YAFARAY

#endif // YAFARAY_IMAGE_H
