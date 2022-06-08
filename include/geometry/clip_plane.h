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

#ifndef YAFARAY_CLIP_PLANE_H
#define YAFARAY_CLIP_PLANE_H

#include "common/yafaray_common.h"
#include "geometry/axis.h"

BEGIN_YAFARAY

struct ClipPlane
{
	enum class Pos: unsigned char { None, Lower, Upper };
	explicit ClipPlane(Pos pos = Pos::None) : pos_(pos) { }
	ClipPlane(Axis axis, Pos pos) : axis_(axis), pos_(pos) { }
	Axis axis_ = Axis::None;
	Pos pos_ = Pos::None;
};

END_YAFARAY

#endif //YAFARAY_CLIP_PLANE_H
