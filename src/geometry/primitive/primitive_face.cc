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

#include "geometry/primitive/primitive_face.h"
#include "geometry/object/object_mesh_bezier.h"

#include <utility>
#include "geometry/uv.h"
#include "geometry/object/object_mesh.h"
#include "geometry/bound.h"
#include "geometry/matrix4.h"

BEGIN_YAFARAY

Point3 FacePrimitive::getVertex(BezierTimeStep time_step, size_t vertex_number) const
{
	//FIXME: ugly static cast but for performance we cannot affort dynamic casts here and the base mesh should be MeshBezier in any case...
	return static_cast<const MeshBezierObject *>(&base_mesh_object_)->getVertex(time_step, vertices_[vertex_number]);
}

END_YAFARAY
