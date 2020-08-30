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

#include "common/bound.h"
#include <iostream>

BEGIN_YAFARAY

Bound::Bound(const Bound &r, const Bound &l)
{
	float minx = std::min(r.a_.x_, l.a_.x_);
	float miny = std::min(r.a_.y_, l.a_.y_);
	float minz = std::min(r.a_.z_, l.a_.z_);
	float maxx = std::max(r.g_.x_, l.g_.x_);
	float maxy = std::max(r.g_.y_, l.g_.y_);
	float maxz = std::max(r.g_.z_, l.g_.z_);
	a_.set(minx, miny, minz);
	g_.set(maxx, maxy, maxz);
}

float Bound::vol() const
{
	float ret = (g_.y_ - a_.y_) * (g_.x_ - a_.x_) * (g_.z_ - a_.z_);

	return ret;
}

END_YAFARAY
