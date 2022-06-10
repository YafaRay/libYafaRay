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

#include "geometry/shape/shape_triangle.h"
#include "geometry/bound.h"
#include "geometry/intersect_data.h"

BEGIN_YAFARAY

IntersectData ShapeTriangle::intersect(const Ray &ray) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Vec3 edge_1{vertices_[1] - vertices_[0]};
	const Vec3 edge_2{vertices_[2] - vertices_[0]};
	const float epsilon = 0.1f * min_raydist_global * std::max(edge_1.length(), edge_2.length());
	const Vec3 pvec{ray.dir_ ^ edge_2};
	const float det = edge_1 * pvec;
	if(det <= -epsilon || det >= epsilon)
	{
		const float inv_det = 1.f / det;
		const Vec3 tvec{ray.from_ - vertices_[0]};
		const float u = (tvec * pvec) * inv_det;
		if(u >= 0.f && u <= 1.f)
		{
			const Vec3 qvec{tvec ^ edge_1};
			const float v = (ray.dir_ * qvec) * inv_det;
			if(v >= 0.f && (u + v) <= 1.f)
			{
				const float t = edge_2 * qvec * inv_det;
				if(t >= epsilon) return IntersectData{t, {u, v}, ray.time_ };
			}
		}
	}
	return {};
}

END_YAFARAY
