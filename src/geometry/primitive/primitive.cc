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

#include "geometry/primitive/primitive.h"
#include "geometry/object/object.h"
#include "geometry/surface.h"
#include "geometry/primitive/primitive_triangle.h"
#include "geometry/primitive/primitive_sphere.h"
#include "geometry/bound.h"
#include "common/logger.h"
#include "common/param.h"

BEGIN_YAFARAY

PolyDouble::ClipResultWithBound Primitive::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly) const
{
	return PolyDouble::ClipResultWithBound(PolyDouble::ClipResultWithBound::Code::FatalError);
}

PolyDouble::ClipResultWithBound Primitive::clipToBound(Logger &logger, const std::array<Vec3Double, 2> &bound, const ClipPlane &clip_plane, const PolyDouble &poly, const Matrix4 &obj_to_world) const
{
	return PolyDouble::ClipResultWithBound(PolyDouble::ClipResultWithBound::Code::FatalError);
}

END_YAFARAY
