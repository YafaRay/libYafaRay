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

#ifndef YAFARAY_IMAGE_GRAY_WEIGHT_H
#define YAFARAY_IMAGE_GRAY_WEIGHT_H

#include "image/image.h"
#include "image/image_buffers.h"

BEGIN_YAFARAY

//!< Grayscale weighted with alpha float (96 bit/pixel) image buffer
class LIBYAFARAY_EXPORT ImageGrayWeight final : public Image
{
	public:
		ImageGrayWeight(int width, int height) : Image(width, height), buffer_{width, height} { }

	private:
		virtual Type getType() const override { return Type::GrayWeight; }
		virtual Image::Optimization getOptimization() const override { return Image::Optimization::None; }
		virtual Rgba getColor(int x, int y) const override { return buffer_(x, y).getColor(); }
		virtual float getFloat(int x, int y) const override { return buffer_(x, y).getFloat(); }
		virtual float getWeight(int x, int y) const override { return buffer_(x, y).getWeight(); }
		virtual int getInt(int x, int y) const override { return 0; }
		virtual void setColor(int x, int y, const Rgba &col) override { buffer_(x, y).setColor(col); }
		virtual void setFloat(int x, int y, float val) override { buffer_(x, y).setFloat(val); }
		virtual void setWeight(int x, int y, float val) override { buffer_(x, y).setWeight(val); }
		virtual void setInt(int x, int y, int val) override { }
		virtual void clear() override { buffer_.clear(); }

		ImageBuffer2D<PixelGray> buffer_;
};

END_YAFARAY

#endif //YAFARAY_IMAGE_GRAY_WEIGHT_H
