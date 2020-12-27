#pragma once
/****************************************************************************
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
 */

#ifndef YAFARAY_VECTOR_DOUBLE_H
#define YAFARAY_VECTOR_DOUBLE_H

#include "constants.h"
#include "geometry/axis.h"
#include <sstream>
#include <iomanip>
#include <limits>

BEGIN_YAFARAY

class Vec3Double
{
	public:
		Vec3Double &operator = (const Vec3Double &v) { x_ = v.x_, y_ = v.y_, z_ = v.z_; return *this; }
		double operator[](int i) const { return (&x_)[i]; }
		double &operator[](int i) { return (&x_)[i]; }
		std::string print() const;
		double x_, y_, z_;
};

inline std::string Vec3Double::print() const
{
	std::stringstream ss;
	ss << std::setprecision(std::numeric_limits<double>::digits10 + 1);
	ss << "<x=" <<  x_ << ",y=" << y_ << ",z=" << z_ << ">";
	return ss.str();
}

END_YAFARAY

#endif //YAFARAY_VECTOR_DOUBLE_H
