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

namespace yafaray {

//!< Optimized grayscale (8 bit/pixel) image buffer
class ImageGrayOptimized final : public Image
{
	public:
		ImageGrayOptimized(const Size2i &size) : Image{size}, buffer_{size} { }

	private:
		Type getType() const override { return Type::Gray; }
		Image::Optimization getOptimization() const override { return Image::Optimization::Optimized; }
		Rgba getColor(const Point2i &point) const override { return buffer_(point).getColor(); }
		float getFloat(const Point2i &point) const override { return getColor(point).r_; }
		void setColor(const Point2i &point, const Rgba &col) override { buffer_(point).setColor(col); }
		void setColor(const Point2i &point, Rgba &&col) override { buffer_(point).setColor(std::move(col)); }
		void addColor(const Point2i &point, const Rgba &col) override { buffer_(point).addColor(col); } // Do not use, this class has too little precision for additions
		void setFloat(const Point2i &point, float val) override { setColor(point, Rgba{val}); }
		void addFloat(const Point2i &point, float val) override { addColor(point, Rgba{val}); } // Do not use, this class has too little precision for additions
		void clear() override { buffer_.clear(); }

		ImageBuffer2D<Gray8> buffer_;
};

} //namespace yafaray

#endif //YAFARAY_IMAGE_GRAY_OPTIMIZED_H
