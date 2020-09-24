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
#include "image_buffers.h"

BEGIN_YAFARAY

enum class ImageType : int;

class Image final
{
	public:
		enum class Optimization : int { None = 1, Optimized = 2, Compressed = 3 };
		Image(int width, int height, const ImageType &type, const Optimization &optimization);
		Image(const Image &image) = delete;
		Image(Image &&image);
		~Image();

		int getWidth() const { return width_; }
		int getHeight() const { return height_; }
		ImageType getType() const { return type_; }
		Image getDenoisedLdrBuffer(float h_col, float h_lum, float mix) const; //!< Provides a denoised buffer, but only works with LDR images (that can be represented in 8-bit 0..255 values). If attempted with HDR images they would lose the HDR range and become unusable!
		Rgba getColor(int x, int y) const;
		float getFloat(int x, int y) const;
		float getWeight(int x, int y) const;
		int getInt(int x, int y) const;
		void setColor(int x, int y, const Rgba &col);
		void setColor(int x, int y, const Rgba &col, ColorSpace color_space, float gamma);
		void setFloat(int x, int y, float val);
		void setWeight(int x, int y, float val);
		void setInt(int x, int y, int val);
		void clear();

	protected:
		int width_;
		int height_;
		ImageType type_;
		Optimization optimization_;
		void *buffer_ = nullptr;
};


inline void Image::setColor(int x, int y, const Rgba &col, ColorSpace color_space, float gamma)
{
	if(color_space == LinearRgb || (color_space == RawManualGamma && gamma == 1.f))
	{
		setColor(x, y, col);
	}
	else
	{
		Rgba col_linear = col;
		col_linear.linearRgbFromColorSpace(color_space, gamma);
		setColor(x, y, col_linear);
	}
}

END_YAFARAY

#endif // YAFARAY_IMAGE_H
