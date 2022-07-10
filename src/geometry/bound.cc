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

template class Bound<float>;

template<typename T>
Bound<T>::Bound(const Bound &r, const Bound &l) :
	a_{{
			std::min(r.a_[Axis::X], l.a_[Axis::X]),
			std::min(r.a_[Axis::Y], l.a_[Axis::Y]),
			std::min(r.a_[Axis::Z], l.a_[Axis::Z])
	}},
	g_{{
			std::max(r.g_[Axis::X], l.g_[Axis::X]),
			std::max(r.g_[Axis::Y], l.g_[Axis::Y]),
			std::max(r.g_[Axis::Z], l.g_[Axis::Z])
	}}
{
	//Empty
}

template<typename T>
T Bound<T>::vol() const
{
	const T ret{(g_[Axis::Y] - a_[Axis::Y]) * (g_[Axis::X] - a_[Axis::X]) * (g_[Axis::Z] - a_[Axis::Z])};
	return ret;
}

} //namespace yafaray
