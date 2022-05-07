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

#include "geometry/primitive/primitive_quad_bezier.h"
#include <memory>
#include "geometry/ray.h"
#include "geometry/bound.h"
#include "geometry/surface.h"
#include "geometry/object/object_mesh_bezier.h"
#include "geometry/shape/shape_quad.h"
#include "geometry/uv.h"
#include "geometry/primitive/primitive_triangle.h"
#include "geometry/shape/shape_triangle.h"
#include "math/interpolation.h"

BEGIN_YAFARAY

ShapeQuad QuadBezierPrimitive::getShapeQuadAtTime(float time, const Matrix4 *obj_to_world) const
{
	//FIXME: ugly static casts but for performance we cannot affort dynamic casts here and the base mesh should be MeshBezier in any case...
	const float time_start = static_cast<const MeshBezierObject *>(&base_mesh_object_)->getTimeRangeStart();
	const float time_end = static_cast<const MeshBezierObject *>(&base_mesh_object_)->getTimeRangeEnd();

	if(time <= time_start) return ShapeQuad {{
			getVertex(BezierTimeStep::Start, 0, obj_to_world),
			getVertex(BezierTimeStep::Start, 1, obj_to_world),
			getVertex(BezierTimeStep::Start, 2, obj_to_world),
			getVertex(BezierTimeStep::Start, 3, obj_to_world)
	}};
	else if(time >= time_end) return ShapeQuad {{
				getVertex(BezierTimeStep::End, 0, obj_to_world),
				getVertex(BezierTimeStep::End, 1, obj_to_world),
				getVertex(BezierTimeStep::End, 2, obj_to_world),
				getVertex(BezierTimeStep::End, 3, obj_to_world)
	}};
	else
	{
		const std::array<Point3, 3> pa {
				getVertex(BezierTimeStep::Start, 0, obj_to_world),
				getVertex(BezierTimeStep::Mid, 0, obj_to_world),
				getVertex(BezierTimeStep::End, 0, obj_to_world)
		};
		const std::array<Point3, 3> pb {
				getVertex(BezierTimeStep::Start, 1, obj_to_world),
				getVertex(BezierTimeStep::Mid, 1, obj_to_world),
				getVertex(BezierTimeStep::End, 1, obj_to_world)
		};
		const std::array<Point3, 3> pc {
				getVertex(BezierTimeStep::Start, 2, obj_to_world),
				getVertex(BezierTimeStep::Mid, 2, obj_to_world),
				getVertex(BezierTimeStep::End, 2, obj_to_world)
		};
		const std::array<Point3, 3> pd {
				getVertex(BezierTimeStep::Start, 3, obj_to_world),
				getVertex(BezierTimeStep::Mid, 3, obj_to_world),
				getVertex(BezierTimeStep::End, 3, obj_to_world)
		};
		const float time_mapped = math::lerpSegment(time, 0.f, time_start, 1.f, time_end); //time_mapped must be in range [0.f-1.f]
		const float tc = 1.f - time_mapped;
		const float b_1 = tc * tc;
		const float b_2 = 2.f * time_mapped * tc;
		const float b_3 = time_mapped * time_mapped;
		return ShapeQuad{{
				b_1 * pa[BezierTimeStep::Start] + b_2 * pa[BezierTimeStep::Mid] + b_3 * pa[BezierTimeStep::End],
				b_1 * pb[BezierTimeStep::Start] + b_2 * pb[BezierTimeStep::Mid] + b_3 * pb[BezierTimeStep::End],
				b_1 * pc[BezierTimeStep::Start] + b_2 * pc[BezierTimeStep::Mid] + b_3 * pc[BezierTimeStep::End],
				b_1 * pd[BezierTimeStep::Start] + b_2 * pd[BezierTimeStep::Mid] + b_3 * pd[BezierTimeStep::End]
		}};
	}
}

IntersectData QuadBezierPrimitive::intersect(const Ray &ray, const Matrix4 *obj_to_world) const
{
	return getShapeQuadAtTime(ray.time_, obj_to_world).intersect(ray);
}

float QuadBezierPrimitive::surfaceArea(const Matrix4 *obj_to_world) const
{
	return ShapeQuad {{ getVertex(BezierTimeStep::Start, 0, obj_to_world), getVertex(BezierTimeStep::Start, 1, obj_to_world), getVertex(BezierTimeStep::Start, 2, obj_to_world), getVertex(BezierTimeStep::Start, 3, obj_to_world) }}.surfaceArea();
}

std::pair<Point3, Vec3> QuadBezierPrimitive::sample(float s_1, float s_2, const Matrix4 *obj_to_world) const
{
	return {
			ShapeQuad{{getVertex(BezierTimeStep::Start, 0, obj_to_world), getVertex(BezierTimeStep::Start, 1, obj_to_world), getVertex(BezierTimeStep::Start, 2, obj_to_world), getVertex(BezierTimeStep::Start, 3, obj_to_world)}}.sample(s_1, s_2),
			Primitive::getGeometricFaceNormal(obj_to_world)
	};
}

void QuadBezierPrimitive::calculateGeometricFaceNormal()
{
	face_normal_geometric_ = getShapeQuadAtTime(0.f, nullptr).calculateFaceNormal();
}

END_YAFARAY
