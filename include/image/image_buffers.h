#pragma once
/****************************************************************************
 *      image_buffers.h: An image buffer handler
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
 *      Copyright (C) 2015-2017 David Bluecame for rgba8888, rgba7773, rgb888,
 *      rgb565, rgb101010 and rgba1010108 buffers for texture RAM optimization
 *      (pixel_t class wa originally written by someone else, don't know exactly who)
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

#ifndef YAFARAY_IMAGE_BUFFERS_H
#define YAFARAY_IMAGE_BUFFERS_H

#include "color/color.h"
#include "math/buffer.h"

BEGIN_YAFARAY

/*! Color weighted pixel structure */
class Pixel final
{
	public:
		Rgba normalized() const
		{
			if(weight_ != 0.f) return col_ / weight_;	//Changed from if(weight > 0.f) to if(weight != 0.f) because lanczos and mitchell filters, as they have a negative lobe, sometimes generate pixels with all negative values and also negative weight. Having if(weight > 0.f) caused such pixels to be incorrectly set to 0,0,0,0 and were shown as black dots (with alpha=0). Options are: clipping the filter output to values >=0, but they would lose ability to sharpen the image. Other option (applied here) is to allow negative values and normalize them correctly. This solves a problem stated in http://yafaray.org/node/712 but perhaps could cause other artifacts? We have to keep an eye on this to decide the best option.
			else return Rgba(0.f);
		}
		Rgba getColor() const { return col_; }
		float getWeight() const { return weight_; }
		void setColor(const Rgba &col) { col_ = col; }
		void setWeight(float weight) { weight_ = weight; }

	private:
		Rgba col_ = Rgba(0.f);
		float weight_ = 0.f;
};

class Gray
{
	public:
		float getFloat() const { return val_; }
		void setFloat(float val) { val_ = val; }
		void setColor(const Rgba &col) { val_ = (col.r_ + col.g_ + col.b_) / 3.f; }
		Rgba getColor() const { return { val_, 1.f }; }

	protected:
		float val_ = 0.f;
};

class GrayAlpha : public Gray
{
	public:
		void setColor(const Rgba &col) { val_ = (col.r_ + col.g_ + col.b_) / 3.f; alpha_ = col.a_; }
		Rgba getColor() const { return { val_, alpha_ }; }

	protected:
		float alpha_ = 0.f;
};

/*! Gray scale weighted pixel structure */
class PixelGray final : public Gray
{
	public:
		float normalized() const
		{
			if(weight_ > 0.f) return val_ / weight_;
			else return 0.f;
		}
		float getWeight() const { return weight_; }
		void setWeight(float weight) { weight_ = weight; }

	private:
		float weight_ = 0.f;
};

/*! Gray scale weighted pixel structure */
class PixelGrayAlpha final : public GrayAlpha
{
	public:
		float normalized() const
		{
			if(weight_ > 0.f) return getFloat() / weight_;
			else return 0.f;
		}
		float getWeight() const { return weight_; }
		void setWeight(float weight) { weight_ = weight; }

	private:
		float weight_ = 0.f;
};

class RgbAlpha final
{
	public:
		void setColor(const Rgba &col) { rgba_ = col; }
		Rgba getColor() const { return rgba_; }

	private:
		Rgba rgba_ {0.f};
};


class Rgba8888 final
{
	public:
		void setColor(const Rgba &col) { setR((uint8_t)roundf(col.r_ * 255.f)); setG((uint8_t)roundf(col.g_ * 255.f)); setB((uint8_t)roundf(col.b_ * 255.f)); setA((uint8_t)roundf(col.a_ * 255.f)); }

		Rgba getColor() const { return Rgba((float) getR() / 255.f, (float) getG() / 255.f, (float) getB() / 255.f, (float) getA() / 255.f); }

	private:
		void setR(uint8_t red_8) { r_ = red_8; }
		void setG(uint8_t green_8) { g_ = green_8; }
		void setB(uint8_t blue_8) { b_ = blue_8; }
		void setA(uint8_t alpha_8) { a_ = alpha_8; }

		uint8_t getR() const { return r_; }
		uint8_t getG() const { return g_; }
		uint8_t getB() const { return b_; }
		uint8_t getA() const { return a_; }

		uint8_t r_ = 0;
		uint8_t g_ = 0;
		uint8_t b_ = 0;
		uint8_t a_ = 0;
};

class Rgba7773 final
{
		//RGBA7773 lossy 24bit format: rrrr rrra gggg ggga bbbb bbba   "invented" by David Bluecame
		//7 bits for each color, 3 bits for alpha channel

	public:
		void setColor(const Rgba &col) { setR((uint8_t)roundf(col.r_ * 255.f)); setG((uint8_t)roundf(col.g_ * 255.f)); setB((uint8_t)roundf(col.b_ * 255.f));  setA((uint8_t)roundf(col.a_ * 255.f)); }

		Rgba getColor() const { return Rgba((float) getR() / 254.f, (float) getG() / 254.f, (float) getB() / 254.f, (float) getA() / 224.f); } //maximum range is 7bit 0xFE (254) for colors and 3bit 0xE0 (224) for alpha, so I'm scaling acordingly. Loss of color data is happening and scaling may make it worse, but it's the only way of doing this consistently

	private:
		void setR(uint8_t red_8) { ra_ = (ra_ & 0x01) | (red_8 & 0xFE); }
		void setG(uint8_t green_8) { ga_ = (ga_ & 0x01) | (green_8 & 0xFE); }
		void setB(uint8_t blue_8) { ba_ = (ba_ & 0x01) | (blue_8 & 0xFE); }
		void setA(uint8_t alpha_8)
		{
			ra_ = (ra_ & 0xFE) | ((alpha_8 & 0x80) >> 7);
			ga_ = (ga_ & 0xFE) | ((alpha_8 & 0x40) >> 6);
			ba_ = (ba_ & 0xFE) | ((alpha_8 & 0x20) >> 5);
		}

		uint8_t getR() const { return ra_ & 0xFE; }
		uint8_t getG() const { return ga_ & 0xFE; }
		uint8_t getB() const { return ba_ & 0xFE; }
		uint8_t getA() const { return ((ra_ & 0x01) << 7) | ((ga_ & 0x01) << 6) | ((ba_ & 0x01) << 5); }

		uint8_t ra_ = 0x00;		//red + alpha most significant bit
		uint8_t ga_ = 0x00;		//green + alpha centre bit
		uint8_t ba_ = 0x00;		//blue + alpha least significant bit
};

class Rgb888 final
{
	public:
		void setColor(const Rgba &col) { setR((uint8_t)roundf(col.r_ * 255.f)); setG((uint8_t)roundf(col.g_ * 255.f)); setB((uint8_t)roundf(col.b_ * 255.f)); }

		Rgba getColor() const { return Rgba((float) getR() / 255.f, (float) getG() / 255.f, (float) getB() / 255.f, 1.f); }

	private:
		void setR(uint8_t red_8) { r_ = red_8; }
		void setG(uint8_t green_8) { g_ = green_8; }
		void setB(uint8_t blue_8) { b_ = blue_8; }

		uint8_t getR() const { return r_; }
		uint8_t getG() const { return g_; }
		uint8_t getB() const { return b_; }

		uint8_t r_ = 0;
		uint8_t g_ = 0;
		uint8_t b_ = 0;
};

class Gray8 final
{
	public:
		void setColor(const Rgba &col)
		{
			const float f_gray_avg = (col.r_ + col.g_ + col.b_) / 3.f;
			value_ = ((uint8_t)roundf(f_gray_avg * 255.f));
		}

		Rgba getColor() const
		{
			const float f_value = (float) value_ / 255.f;
			return Rgba(f_value, 1.f);
		}

		uint8_t getInt() const { return value_; }
		void setInt(uint8_t val) { value_ = val; }

	private:
		uint8_t value_ = 0;
};


class Rgb565 final
{
		//RGB565 lossy 16bit format: rrrr rggg gggb bbbb
	public:
		void setColor(const Rgba &col) { setR((uint8_t)roundf(col.r_ * 255.f)); setG((uint8_t)roundf(col.g_ * 255.f)); setB((uint8_t)roundf(col.b_ * 255.f)); }

		Rgba getColor() const { return Rgba((float) getR() / 248.f, (float) getG() / 252.f, (float) getB() / 248.f, 1.f); } //maximum range is 5bit 0xF8 (248) for r,b colors and 6bit 0xFC (252) for g color, so I'm scaling acordingly. Loss of color data is happening and scaling may make it worse, but it's the only way of doing this consistently

	private:
		void setR(uint8_t red_8) { rgb_565_ = (rgb_565_ & 0x07FF) | ((red_8 & 0xF8) << 8); }
		void setG(uint8_t green_8) { rgb_565_ = (rgb_565_ & 0xF81F) | ((green_8 & 0xFC) << 3); }
		void setB(uint8_t blue_8) { rgb_565_ = (rgb_565_ & 0xFFE0) | ((blue_8 & 0xF8) >> 3); }

		uint8_t getR() const { return ((rgb_565_ & 0xF800) >> 8); }
		uint8_t getG() const { return ((rgb_565_ & 0x07E0) >> 3); }
		uint8_t getB() const { return ((rgb_565_ & 0x001F) << 3); }

		uint16_t rgb_565_ = 0;
};

class Rgb101010 final
{
		//RGB101010 RGB 32bit extra integer precision format: 00rrggbb rrrrrrrr gggggggg bbbbbbbb
		//Bit location scheme "invented" by David Bluecame
	public:
		void setColor(const Rgba &col)
		{
			setR((uint16_t)roundf(col.r_ * 1023.f));
			setG((uint16_t)roundf(col.g_ * 1023.f));
			setB((uint16_t)roundf(col.b_ * 1023.f));
		}

		Rgba getColor() const { return Rgba((float) getR() / 1023.f, (float) getG() / 1023.f, (float) getB() / 1023.f, 1.f); }

	private:
		void setR(uint16_t red_10) { r_ = (red_10 & 0x00FF); rgb_extra_ = (rgb_extra_ & 0x0F) | ((red_10 & 0x0300) >> 4); }
		void setG(uint16_t green_10) { g_ = (green_10 & 0x00FF); rgb_extra_ = (rgb_extra_ & 0x33) | ((green_10 & 0x0300) >> 6); }
		void setB(uint16_t blue_10) { b_ = (blue_10 & 0x00FF); rgb_extra_ = (rgb_extra_ & 0x3C) | ((blue_10 & 0x0300) >> 8); }

		uint16_t getR() const { return r_ + ((uint16_t)(rgb_extra_ & 0x30) << 4); }
		uint16_t getG() const { return g_ + ((uint16_t)(rgb_extra_ & 0x0C) << 6); }
		uint16_t getB() const { return b_ + ((uint16_t)(rgb_extra_ & 0x03) << 8); }

		uint8_t rgb_extra_ = 0;
		uint8_t r_ = 0;
		uint8_t g_ = 0;
		uint8_t b_ = 0;
};

class Rgba1010108 final
{
		//RGB1010108 RGBA 40bit extra integer precision format: 00rrggbb rrrrrrrr gggggggg bbbbbbbb aaaaaaaaa
		//Bit location scheme "invented" by David Bluecame
	public:
		void setColor(const Rgba &col)
		{
			setR((uint16_t)roundf(col.r_ * 1023.f));
			setG((uint16_t)roundf(col.g_ * 1023.f));
			setB((uint16_t)roundf(col.b_ * 1023.f));
			setA((uint8_t)roundf(col.a_ * 255.f));
		}

		Rgba getColor() const { return Rgba((float) getR() / 1023.f, (float) getG() / 1023.f, (float) getB() / 1023.f, (float) getA() / 255.f); }

	private:
		void setR(uint16_t red_10) { r_ = (red_10 & 0x00FF); rgb_extra_ = (rgb_extra_ & 0x0F) | ((red_10 & 0x0300) >> 4); }
		void setG(uint16_t green_10) { g_ = (green_10 & 0x00FF); rgb_extra_ = (rgb_extra_ & 0x33) | ((green_10 & 0x0300) >> 6); }
		void setB(uint16_t blue_10) { b_ = (blue_10 & 0x00FF); rgb_extra_ = (rgb_extra_ & 0x3C) | ((blue_10 & 0x0300) >> 8); }
		void setA(uint8_t alpha_8) { a_ = alpha_8; }

		uint16_t getR() const { return r_ + ((uint16_t)(rgb_extra_ & 0x30) << 4); }
		uint16_t getG() const { return g_ + ((uint16_t)(rgb_extra_ & 0x0C) << 6); }
		uint16_t getB() const { return b_ + ((uint16_t)(rgb_extra_ & 0x03) << 8); }
		uint8_t getA() const { return a_; }

		uint8_t rgb_extra_ = 0;
		uint8_t r_ = 0;
		uint8_t g_ = 0;
		uint8_t b_ = 0;
		uint8_t a_ = 0;
};

template <class T>
class ImageBuffer2D final : public Buffer<T, 2>
{
	public:
		ImageBuffer2D(int weight, int height) : Buffer<T, 2>{{ static_cast<size_t>(weight), static_cast<size_t>(height) }} { }
		void set(int x, int y, const T &val) { Buffer<T, 2>::set({ static_cast<size_t>(x), static_cast<size_t>(y) }, val); }
		T get(int x, int y) const { return Buffer<T, 2>::get({ static_cast<size_t>(x), static_cast<size_t>(y) }); }
		T &operator()(int x, int y) { return Buffer<T, 2>::operator()({ static_cast<size_t>(x), static_cast<size_t>(y) }); }
		const T &operator()(int x, int y) const { return Buffer<T, 2>::operator()({ static_cast<size_t>(x), static_cast<size_t>(y) }); }
		void clear() { Buffer<T, 2>::zero(); }
		int getWidth() const { return static_cast<int>(Buffer<T, 2>::getDimensions().at(0)); }
		int getHeight() const { return static_cast<int>(Buffer<T, 2>::getDimensions().at(1)); }
};

END_YAFARAY

#endif
