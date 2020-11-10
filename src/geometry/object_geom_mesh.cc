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

#include "geometry/object_geom_mesh.h"
#include "geometry/primitive_triangle.h"
#include "geometry/primitive_triangle_bspline_time.h"
#include "geometry/uv.h"

BEGIN_YAFARAY

MeshObject::MeshObject(int ntris, bool has_uv, bool has_orco):
		has_orco_(has_orco), has_uv_(has_uv)
{
	//triangles.reserve(ntris);
	if(has_uv) uv_offsets_.reserve(ntris);
}

int MeshObject::getPrimitives(const Primitive **prims) const
{
	int n = 0;
	for(unsigned int i = 0; i < v_triangles_.size(); ++i, ++n)
	{
		prims[n] = &(v_triangles_[i]);
	}
	for(unsigned int i = 0; i < bs_triangles_.size(); ++i, ++n)
	{
		prims[n] = &(bs_triangles_[i]);
	}
	return n;
}

Primitive *MeshObject::addTriangle(const VTriangle &t)
{
	v_triangles_.push_back(t);
	return &(v_triangles_.back());
}

Primitive *MeshObject::addBsTriangle(const BsTriangle &t)
{
	bs_triangles_.push_back(t);
	return &(v_triangles_.back());
}

void MeshObject::finish()
{
	for(auto &triangle : v_triangles_) triangle.recNormal();
}

int MeshObject::convertToBezierControlPoints()
{
	const int n = points_.size();
	if(n % 3 == 0)
	{
		//convert point 2 to quadratic bezier control point
		points_[n - 2] = 2.f * points_[n - 2] - 0.5f * (points_[n - 3] + points_[n - 1]);
	}
	return (n - 1) / 3;
}

END_YAFARAY