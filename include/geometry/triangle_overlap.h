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

/*************************************************************
*      The code below (triBoxOverlap and related functions)
*      is based on "AABB-triangle overlap test code"
*          by Tomas Akenine-Möller
*      (see information below about the original code)
************************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],    */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/

#ifndef LIBYAFARAY_TRIANGLE_OVERLAP_H
#define LIBYAFARAY_TRIANGLE_OVERLAP_H

#include "geometry/vector_double.h"

BEGIN_YAFARAY

namespace triangle_overlap
{

struct MinMax
{
	double min_;
	double max_;
	static MinMax find(const Vec3Double &values);
};

inline MinMax MinMax::find(const Vec3Double &values)
{
	MinMax min_max;
	min_max.min_ = math::min(values[Axis::X], values[Axis::Y], values[Axis::Z]);
	min_max.max_ = math::max(values[Axis::X], values[Axis::Y], values[Axis::Z]);
	return min_max;
}

inline int planeBoxOverlap(const Vec3Double &normal, const Vec3Double &vert, const Vec3Double &maxbox)    // -NJMP-
{
	Vec3Double vmin, vmax;
	for(const auto axis : axis::spatial)
	{
		const double v = vert[axis];                    // -NJMP-
		if(normal[axis] > 0)
		{
			vmin[axis] = -maxbox[axis] - v;    // -NJMP-
			vmax[axis] = maxbox[axis] - v;    // -NJMP-
		}
		else
		{
			vmin[axis] = maxbox[axis] - v;    // -NJMP-
			vmax[axis] = -maxbox[axis] - v;    // -NJMP-
		}
	}
	if(Vec3Double::dot(normal, vmin) > 0) return 0;    // -NJMP-
	if(Vec3Double::dot(normal, vmax) >= 0) return 1;    // -NJMP-

	return 0;
}

inline bool axisTest(double a, double b, double f_a, double f_b, const Vec3Double &v_a, const Vec3Double &v_b, const Vec3Double &boxhalfsize, Axis axis)
{
	const Axis axis_a = (axis == Axis::X ? Axis::Y : Axis::X);
	const Axis axis_b = (axis == Axis::Z ? Axis::Y : Axis::Z);
	const int sign = (axis == Axis::Y ? -1 : 1);
	const double p_a = sign * (a * v_a[axis_a] - b * v_a[axis_b]);
	const double p_b = sign * (a * v_b[axis_a] - b * v_b[axis_b]);
	double min, max;
	if(p_a < p_b)
	{
		min = p_a;
		max = p_b;
	}
	else
	{
		min = p_b;
		max = p_a;
	}
	const double rad = f_a * boxhalfsize[axis_a] + f_b * boxhalfsize[axis_b];
	if(min > rad || max < -rad) return false;
	else return true;
}

inline bool triBoxOverlap(const Vec3Double &boxcenter, const Vec3Double &boxhalfsize, const std::array<Vec3Double, 3> &triverts)
{

	/*    use separating axis theorem to test overlap between triangle and box */
	/*    need to test for overlap in these directions: */
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
	/*       we do not even need to test these) */
	/*    2) normal of the triangle */
	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */
	/*       this gives 3x3=9 more tests */
	/* This is the fastest branch on Sun */
	/* move everything so that the boxcenter is in (0,0,0) */
	const std::array<Vec3Double, 3> tri_verts {
			Vec3Double::sub(triverts[0], boxcenter),
			Vec3Double::sub(triverts[1], boxcenter),
			Vec3Double::sub(triverts[2], boxcenter)
	};
	const std::array<Vec3Double, 3> tri_edges {
			Vec3Double::sub(tri_verts[1], tri_verts[0]),
			Vec3Double::sub(tri_verts[2], tri_verts[1]),
			Vec3Double::sub(tri_verts[0], tri_verts[2])
	};
	/* Bullet 3:  */
	/*  test the 9 tests first (this was faster) */
	const std::array<Vec3Double, 3> fe {{
			{std::abs(tri_edges[0][Axis::X]), std::abs(tri_edges[0][Axis::Y]), std::abs(tri_edges[0][Axis::Z])},
			{std::abs(tri_edges[1][Axis::X]), std::abs(tri_edges[1][Axis::Y]), std::abs(tri_edges[1][Axis::Z])},
			{std::abs(tri_edges[2][Axis::X]), std::abs(tri_edges[2][Axis::Y]), std::abs(tri_edges[2][Axis::Z])}
	}};
	if(!triangle_overlap::axisTest(tri_edges[0][Axis::Z], tri_edges[0][Axis::Y], fe[0][Axis::Z], fe[0][Axis::Y], tri_verts[0], tri_verts[2], boxhalfsize, Axis::X)) return false;
	if(!triangle_overlap::axisTest(tri_edges[0][Axis::Z], tri_edges[0][Axis::X], fe[0][Axis::Z], fe[0][Axis::X], tri_verts[0], tri_verts[2], boxhalfsize, Axis::Y)) return false;
	if(!triangle_overlap::axisTest(tri_edges[0][Axis::Y], tri_edges[0][Axis::X], fe[0][Axis::Y], fe[0][Axis::X], tri_verts[1], tri_verts[2], boxhalfsize, Axis::Z)) return false;

	if(!triangle_overlap::axisTest(tri_edges[1][Axis::Z], tri_edges[1][Axis::Y], fe[1][Axis::Z], fe[1][Axis::Y], tri_verts[0], tri_verts[2], boxhalfsize, Axis::X)) return false;
	if(!triangle_overlap::axisTest(tri_edges[1][Axis::Z], tri_edges[1][Axis::X], fe[1][Axis::Z], fe[1][Axis::X], tri_verts[0], tri_verts[2], boxhalfsize, Axis::Y)) return false;
	if(!triangle_overlap::axisTest(tri_edges[1][Axis::Y], tri_edges[1][Axis::X], fe[1][Axis::Y], fe[1][Axis::X], tri_verts[0], tri_verts[1], boxhalfsize, Axis::Z)) return false;

	if(!triangle_overlap::axisTest(tri_edges[2][Axis::Z], tri_edges[2][Axis::Y], fe[2][Axis::Z], fe[2][Axis::Y], tri_verts[0], tri_verts[1], boxhalfsize, Axis::X)) return false;
	if(!triangle_overlap::axisTest(tri_edges[2][Axis::Z], tri_edges[2][Axis::X], fe[2][Axis::Z], fe[2][Axis::X], tri_verts[0], tri_verts[1], boxhalfsize, Axis::Y)) return false;
	if(!triangle_overlap::axisTest(tri_edges[2][Axis::Y], tri_edges[2][Axis::X], fe[2][Axis::Y], fe[2][Axis::X], tri_verts[1], tri_verts[2], boxhalfsize, Axis::Z)) return false;

	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in the 3 directions */
	for(const auto axis : axis::spatial)
	{
		const MinMax min_max = MinMax::find(tri_verts[axis::getId(axis)]);
		if(min_max.min_ > boxhalfsize[axis] || min_max.max_ < -boxhalfsize[axis]) return false;
	}

	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	const Vec3Double normal = Vec3Double::cross(tri_edges[0], tri_edges[1]);
	// -NJMP- (line removed here)
	if(!triangle_overlap::planeBoxOverlap(normal, tri_verts[0], boxhalfsize)) return false;	// -NJMP-

	return true;   /* box and triangle overlaps */
}

} //namespace triangle_overlap

END_YAFARAY

#endif//LIBYAFARAY_TRIANGLE_OVERLAP_H
