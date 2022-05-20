/****************************************************************************
 *
 *      color.cc: Color type and operators implementation
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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
#include "color/color.h"
#include "math/interpolation.h"
#include <iostream>

BEGIN_YAFARAY

void operator >> (const unsigned char *data, Rgb &c)
{
	c.r_ = static_cast<float>(data[0]) / static_cast<float>(255);
	c.g_ = static_cast<float>(data[1]) / static_cast<float>(255);
	c.b_ = static_cast<float>(data[2]) / static_cast<float>(255);
}

void operator << (unsigned char *data, const Rgb &c)
{
	data[0] = (c.r_ < 0.f) ? 0 : ((c.r_ >= 1.f) ? 255 : static_cast<unsigned char>(255.f * c.r_));
	data[1] = (c.g_ < 0.f) ? 0 : ((c.g_ >= 1.f) ? 255 : static_cast<unsigned char>(255.f * c.g_));
	data[2] = (c.b_ < 0.f) ? 0 : ((c.b_ >= 1.f) ? 255 : static_cast<unsigned char>(255.f * c.b_));
}

void operator >> (const unsigned char *data, Rgba &c)
{
	c.r_ = static_cast<float>(data[0]) / static_cast<float>(255);
	c.g_ = static_cast<float>(data[1]) / static_cast<float>(255);
	c.b_ = static_cast<float>(data[2]) / static_cast<float>(255);
	c.a_ = static_cast<float>(data[3]) / static_cast<float>(255);
}


void operator << (unsigned char *data, const Rgba &c)
{
	data[0] = (c.r_ < 0.f) ? 0 : ((c.r_ >= 1.f) ? 255 : static_cast<unsigned char>(255.f * c.r_));
	data[1] = (c.g_ < 0.f) ? 0 : ((c.g_ >= 1.f) ? 255 : static_cast<unsigned char>(255.f * c.g_));
	data[2] = (c.b_ < 0.f) ? 0 : ((c.b_ >= 1.f) ? 255 : static_cast<unsigned char>(255.f * c.b_));
	data[3] = (c.a_ < 0.f) ? 0 : ((c.a_ >= 1.f) ? 255 : static_cast<unsigned char>(255.f * c.a_));
}

void operator >> (const float *data, Rgb &c)
{
	c.r_ = data[0];
	c.g_ = data[1];
	c.b_ = data[2];
}

void operator << (float *data, const Rgb &c)
{
	data[0] = c.r_;
	data[1] = c.g_;
	data[2] = c.b_;
}

void operator >> (const float *data, Rgba &c)
{
	c.r_ = data[0];
	c.g_ = data[1];
	c.b_ = data[2];
	c.a_ = data[3];
}

void operator << (float *data, const Rgba &c)
{
	data[0] = c.r_;
	data[1] = c.g_;
	data[2] = c.b_;
	data[3] = c.a_;
}

std::ostream &operator << (std::ostream &out, const Rgb &c)
{
	out << "[" << c.r_ << " " << c.g_ << " " << c.b_ << "]";
	return out;
}

std::ostream &operator << (std::ostream &out, const Rgba &c)
{
	out << "[" << c.r_ << ", " << c.g_ << ", " << c.b_ << ", " << c.a_ << "]";
	return out;
}

Rgb Rgb::mix(const Rgb &a, const Rgb &b, float point)
{
	return math::lerpTruncated(b, a, point);
}

Rgba Rgba::mix(const Rgba &a, const Rgba &b, float point)
{
	return math::lerpTruncated(b, a, point);
}

std::string Rgb::colorSpaceName(const ColorSpace &color_space)
{
	switch(color_space)
	{
		case ColorSpace::RawManualGamma: return "Raw_Manual_Gamma";
		case ColorSpace::LinearRgb: return "LinearRGB";
		case ColorSpace::Srgb: return "sRGB";
		case ColorSpace::XyzD65: return "XYZ";
		default: return "none/unknown";
	}
}

ColorSpace Rgb::colorSpaceFromName(const std::string &color_space_name, const ColorSpace &default_color_space)
{
	if(color_space_name == "Raw_Manual_Gamma") return ColorSpace::RawManualGamma;
	else if(color_space_name == "LinearRGB") return ColorSpace::LinearRgb;
	else if(color_space_name == "sRGB") return ColorSpace::Srgb;
	else if(color_space_name == "XYZ") return ColorSpace::XyzD65;
	else return default_color_space;
}

END_YAFARAY
