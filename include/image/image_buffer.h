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

#ifndef YAFARAY_IMAGE_BUFFER_H
#define YAFARAY_IMAGE_BUFFER_H

#include "image/image.h"
#include "math/buffer_2d.h"
#include "image/image_pixel_types.h"

namespace yafaray {

template <typename T>
class ImageBuffer final : public Image
{
	public:
		explicit ImageBuffer(const Params &params) : Image{params} { }

	private:
		Type getType() const override;
		Optimization getOptimization() const override;
		Rgba getColor(const Point2i &point) const override;
		float getFloat(const Point2i &point) const override;
		void setColor(const Point2i &point, const Rgba &col) override;
		void setColor(const Point2i &point, Rgba &&col) override;
		void addColor(const Point2i &point, const Rgba &col) override; //<! Avoid when using optimized or compressed buffers, not enough precision for additions
		void setFloat(const Point2i &point, float val) override;
		void addFloat(const Point2i &point, float val) override; //<! Avoid when using optimized or compressed buffers, not enough precision for additions
		void clear() override;

		Buffer2D<T> buffer_{Size2i{{params_.width_, params_.height_}}};
};

template <typename T>
inline Image::Type ImageBuffer<T>::getType() const
{
	if constexpr (std::is_same_v<T, Rgb>) return Type{Type::Color};
	else if constexpr (std::is_same_v<T, RgbAlpha> || std::is_same_v<T, Rgba7773> || std::is_same_v<T, Rgba1010108>) return Type{Type::ColorAlpha};
	else if constexpr (std::is_same_v<T, Gray> || std::is_same_v<T, Gray8>) return Type{Type::Gray};
	else if constexpr (std::is_same_v<T, GrayAlpha>) return Type{Type::GrayAlpha};
	else return Type{Type::None};
}

template <typename T>
inline Image::Optimization ImageBuffer<T>::getOptimization() const
{
	if constexpr (std::is_same_v<T, Rgba7773> || std::is_same_v<T, Rgb565>) return Optimization{Optimization::Compressed};
	else if constexpr (std::is_same_v<T, Rgba1010108> || std::is_same_v<T, Rgb101010> || std::is_same_v<T, Gray8>) return Optimization{Optimization::Optimized};
	else return Optimization{Optimization::None};
}

template <typename T>
inline Rgba ImageBuffer<T>::getColor(const Point2i &point) const
{
	if constexpr (std::is_same_v<T, Rgb>) return Rgba{buffer_(point)};
	else return buffer_(point).getColor();
}

template <typename T>
inline float ImageBuffer<T>::getFloat(const Point2i &point) const
{
	if constexpr (std::is_same_v<T, Gray> || std::is_same_v<T, GrayAlpha>) return buffer_(point).getFloat();
	else return getColor(point).r_;
}

template <typename T>
inline void ImageBuffer<T>::setColor(const Point2i &point, const Rgba &col)
{
	if constexpr (std::is_same_v<T, Rgb>) buffer_(point) = col;
	else buffer_(point).setColor(col);
}

template <typename T>
inline void ImageBuffer<T>::setColor(const Point2i &point, Rgba &&col)
{
	if constexpr (std::is_same_v<T, Rgb>) buffer_(point) = std::move(col);
	else buffer_(point).setColor(std::move(col));
}

template <typename T>
inline void ImageBuffer<T>::addColor(const Point2i &point, const Rgba &col)
{
	if constexpr (std::is_same_v<T, Rgb>) buffer_(point) += col;
	else buffer_(point).addColor(col);
}

template <typename T>
inline void ImageBuffer<T>::setFloat(const Point2i &point, float val)
{
	if constexpr (std::is_same_v<T, Gray> || std::is_same_v<T, GrayAlpha>) buffer_(point).setFloat(val);
	else setColor(point, Rgba{val});
}

template <typename T>
inline void ImageBuffer<T>::addFloat(const Point2i &point, float val)
{
	if constexpr (std::is_same_v<T, Gray> || std::is_same_v<T, GrayAlpha>) buffer_(point).addFloat(val);
	else addColor(point, Rgba{val});
}

template <typename T>
inline void ImageBuffer<T>::clear()
{
	buffer_.clear();
}

} //namespace yafaray

#endif //YAFARAY_IMAGE_BUFFER_H
