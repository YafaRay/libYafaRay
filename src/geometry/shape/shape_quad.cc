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
#include "geometry/shape/shape_quad.h"
#include "geometry/bound.h"
#include "geometry/intersect_data.h"
#include "geometry/triangle_overlap.h"

BEGIN_YAFARAY

IntersectData ShapeQuad::intersect(const Ray &ray) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Vec3 edge_1{vertices_[1] - vertices_[0]};
	const Vec3 edge_2{vertices_[2] - vertices_[0]};
	const float edge_1_length = edge_1.length();
	const float edge_2_length = edge_2.length();
	const float epsilon_1_2 = 0.1f * min_raydist_global * std::max(edge_1_length, edge_2_length);
	const Vec3 pvec_2{ray.dir_ ^ edge_2};
	const float det_1_2 = edge_1 * pvec_2;
	if(det_1_2 <= -epsilon_1_2 || det_1_2 >= epsilon_1_2) //Test first triangle in the quad
	{
		const float inv_det_1_2 = 1.f / det_1_2;
		const Vec3 tvec{ray.from_ - vertices_[0]};
		float u = (tvec * pvec_2) * inv_det_1_2;
		if(u >= 0.f && u <= 1.f)
		{
			const Vec3 qvec_1{tvec ^ edge_1};
			const float v = (ray.dir_ * qvec_1) * inv_det_1_2;
			if(v >= 0.f && (u + v) <= 1.f)
			{
				const float t = edge_2 * qvec_1 * inv_det_1_2;
				if(t >= epsilon_1_2)
				{
					IntersectData intersect_data;
					intersect_data.hit_ = true;
					intersect_data.t_hit_ = t;
					intersect_data.u_ = u + v;
					intersect_data.v_ = v;
					intersect_data.time_ = ray.time_;
					return intersect_data;
				}
			}
		}
		else //Test second triangle in the quad
		{
			const Vec3 edge_3{vertices_[3] - vertices_[0]};
			const float edge_3_length = edge_3.length();
			const float epsilon_2_3 = 0.1f * min_raydist_global * std::max(edge_2_length, edge_3_length);
			const Vec3 pvec_3{ray.dir_ ^ edge_3};
			const float det_2_3 = edge_2 * pvec_3;
			if(det_2_3 <= -epsilon_2_3 || det_2_3 >= epsilon_2_3)
			{
				const float inv_det_2_3 = 1.f / det_2_3;
				u = (tvec * pvec_3) * inv_det_2_3;
				if(u >= 0.f && u <= 1.f)
				{
					const Vec3 qvec_2{tvec ^ edge_2};
					const float v = (ray.dir_ * qvec_2) * inv_det_2_3;
					if(v >= 0.f && (u + v) <= 1.f)
					{
						const float t = edge_3 * qvec_2 * inv_det_2_3;
						if(t >= epsilon_2_3)
						{
							IntersectData intersect_data;
							intersect_data.hit_ = true;
							intersect_data.t_hit_ = t;
							intersect_data.u_ = u;
							intersect_data.v_ = v + u;
							intersect_data.time_ = ray.time_;
							return intersect_data;
						}
					}
				}
			}
		}
	}
	return {};
}

float ShapeQuad::surfaceArea() const
{
	return ShapeTriangle{{vertices_[0], vertices_[1], vertices_[2]}}.surfaceArea() + ShapeTriangle{{vertices_[0], vertices_[2], vertices_[3]}}.surfaceArea();
}

END_YAFARAY