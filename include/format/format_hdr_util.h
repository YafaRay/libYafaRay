#pragma once
/****************************************************************************
 *
 *      hdrUtils.h: Radiance RGBE format utilities
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 George Laskowsky Ziguilinsky (Glaskows)
 *      			   Rodrigo Placencia Vazquez (DarkTide)
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

#ifndef YAFARAY_FORMAT_HDR_UTIL_H
#define YAFARAY_FORMAT_HDR_UTIL_H

#include "color/color.h"
#include "common/string.h"

BEGIN_YAFARAY

#pragma pack(push, 1)

class RgbeHeader final
{
	public:
		RgbeHeader();

		float exposure_; // in an image corresponds to <exposure> watts/steradian/m^2. Defaults to 1.0
		std::string program_type_; // A string that usually contains "RADIANCE"
		int min_[2], max_[2], step_[2]; // Image boundaries and iteration stepping
		bool y_first_; // Indicates if the image scanlines are saved starting by y axis (Default: true)
};

inline RgbeHeader::RgbeHeader()
{
	program_type_ = "RADIANCE";
	exposure_ = 1.0f;
	y_first_ = true;
}

class RgbePixel final
{
	public:
		RgbePixel &operator = (const Rgb &c);
		RgbePixel &operator = (const RgbePixel &c);
		uint8_t &operator [](int i) { return (&r_)[i]; }
		Rgba getRgba() const;
		bool isOrleDesc() const { return ((r_ == 1) && (g_ == 1) && (b_ == 1)); }
		bool isArleDesc() const { return ((r_ == 2) && (g_ == 2) && ((int)(b_ << 8 | e_) < 0x8000)); }
		int getOrleCount(int rshift) const { return (static_cast<int>(e_) << rshift); }
		int getArleCount() const { return (static_cast<int>(b_ << 8 | e_)); }
		void setScanlineStart(int w);

	private:
		uint8_t r_;
		uint8_t g_;
		uint8_t b_;
		uint8_t e_;
};

inline RgbePixel &RgbePixel::operator=(const Rgb &c)
{
	float v = c.maximum();
	if(v < 1e-32) r_ = g_ = b_ = e_ = 0;
	else
	{
		int e;
		v = std::frexp(v, &e) * 255.9999f / v;
		r_ = static_cast<uint8_t>(c.getR() * v);
		g_ = static_cast<uint8_t>(c.getG() * v);
		b_ = static_cast<uint8_t>(c.getB() * v);
		e_ = static_cast<uint8_t>(e + 128);
	}
	return *this;
}

inline RgbePixel &RgbePixel::operator=(const RgbePixel &c)
{
	r_ = c.r_;
	g_ = c.g_;
	b_ = c.b_;
	e_ = c.e_;
	return *this;
}

inline Rgba RgbePixel::getRgba() const
{
	if(e_)
	{
		/*nonzero pixel*/
		const float f = math::ldexp(1.f, e_ - (128 + 8));
		return Rgba(f * r_, f * g_, f * b_, 1.f);
	}
	return Rgba(0.f, 0.f, 0.f, 1.0f);
}

inline void RgbePixel::setScanlineStart(int w)
{
	r_ = 2;
	g_ = 2;
	b_ = w >> 8;
	e_ = w & 0xFF;
}

#pragma pack(pop)

END_YAFARAY

#endif // YAFARAY_FORMAT_HDR_UTIL_H