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

#ifndef YAFARAY_IMAGE_GRAY_OPTIMIZED_H
#define YAFARAY_IMAGE_GRAY_OPTIMIZED_H

#include "image/image.h"
#include "image/image_buffers.h"

BEGIN_YAFARAY

//!< Optimized grayscale (8 bit/pixel) image buffer
class ImageGrayOptimized final : public Image
{
	public:
		ImageGrayOptimized(int width, int height) : Image(width, height), buffer_{width, height} { }

	private:
		Type getType() const override { return Type::Gray; }
		Image::Optimization getOptimization() const override { return Image::Optimization::Optimized; }
		Rgba getColor(int x, int y) const override { return buffer_(x, y).getColor(); }
		float getFloat(int x, int y) const override { return getColor(x, y).r_; }
		void setColor(int x, int y, const Rgba &col) override { buffer_(x, y).setColor(col); }
		void setColor(int x, int y, Rgba &&col) override { buffer_(x, y).setColor(std::move(col)); }
		void addColor(int x, int y, const Rgba &col) override { buffer_(x, y).addColor(col); } // Do not use, this class has too little precision for additions
		void setFloat(int x, int y, float val) override { setColor(x, y, Rgba{val}); }
		void addFloat(int x, int y, float val) override { addColor(x, y, Rgba{val}); } // Do not use, this class has too little precision for additions
		void clear() override { buffer_.clear(); }

		ImageBuffer2D<Gray8> buffer_;
};

END_YAFARAY

#endif //YAFARAY_IMAGE_GRAY_OPTIMIZED_H
