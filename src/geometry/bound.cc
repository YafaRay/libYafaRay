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

#include "geometry/bound.h"

namespace yafaray {

Bound::Bound(const Bound &r, const Bound &l) :
	a_{
			std::min(r.a_.x(), l.a_.x()),
			std::min(r.a_.y(), l.a_.y()),
			std::min(r.a_.z(), l.a_.z())
	},
	g_{
			std::max(r.g_.x(), l.g_.x()),
			std::max(r.g_.y(), l.g_.y()),
			std::max(r.g_.z(), l.g_.z())
	}
{
	//Empty
}

float Bound::vol() const
{
	const float ret = (g_.y() - a_.y()) * (g_.x() - a_.x()) * (g_.z() - a_.z());
	return ret;
}

} //namespace yafaray
