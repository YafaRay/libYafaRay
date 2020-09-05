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

#ifndef YAFARAY_IMAGEHANDLER_UTIL_HDR_H
#define YAFARAY_IMAGEHANDLER_UTIL_HDR_H

#include "common/color.h"
#include "utility/util_string.h"

BEGIN_YAFARAY

#pragma pack(push, 1)

struct RgbeHeader
{
	RgbeHeader()
	{
		program_type_ = "RADIANCE";
		exposure_ = 1.0f;
		y_first_ = true;
	}

	float exposure_; // in an image corresponds to <exposure> watts/steradian/m^2. Defaults to 1.0
	std::string program_type_; // A string that usually contains "RADIANCE"
	int min_[2], max_[2], step_[2]; // Image boundaries and iteration stepping
	bool y_first_; // Indicates if the image scanlines are saved starting by y axis (Default: true)
};

struct RgbePixel
{
	RgbePixel &operator = (const Rgb &c)
	{
		int e;
		float v = c.maximum();

		if(v < 1e-32)
		{
			r_ = g_ = b_ = e_ = 0;
		}
		else
		{
			v = frexp(v, &e) * 255.9999 / v;
			r_ = (uint8_t)(c.getR() * v);
			g_ = (uint8_t)(c.getG() * v);
			b_ = (uint8_t)(c.getB() * v);
			e_ = (uint8_t)(e + 128);
		}

		return *this;
	}

	RgbePixel &operator = (const RgbePixel &c)
	{
		r_ = c.r_;
		g_ = c.g_;
		b_ = c.b_;
		e_ = c.e_;

		return *this;
	}

	uint8_t &operator [](int i)
	{
		return (&r_)[i];
	}

	Rgba getRgba() const
	{
		float f;

		if(e_)
		{
			/*nonzero pixel*/
			f = fLdexp__(1.0, e_ - (int) (128 + 8));
			return Rgba(f * r_, f * g_, f * b_, 1.0f);
		}

		return Rgba(0.f, 0.f, 0.f, 1.0f);
	}

	bool isOrleDesc()
	{
		return ((r_ == 1) && (g_ == 1) && (b_ == 1));
	}

	bool isArleDesc()
	{
		return ((r_ == 2) && (g_ == 2) && ((int)(b_ << 8 | e_) < 0x8000));
	}

	int getOrleCount(int rshift)
	{
		return ((int)e_ << rshift);
	}

	int getArleCount()
	{
		return ((int)(b_ << 8 | e_));
	}

	void setScanlineStart(int w)
	{
		r_ = 2;
		g_ = 2;
		b_ = w >> 8;
		e_ = w & 0xFF;
	}

	uint8_t r_;
	uint8_t g_;
	uint8_t b_;
	uint8_t e_;
};

#pragma pack(pop)

END_YAFARAY

#endif // YAFARAY_IMAGEHANDLER_UTIL_HDR_H