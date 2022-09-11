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

namespace yafaray {

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

} //namespace yafaray
