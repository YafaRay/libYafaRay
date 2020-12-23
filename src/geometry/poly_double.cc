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

#include "geometry/poly_double.h"
#include "geometry/axis.h"
#include "common/logger.h"
#include <array>

BEGIN_YAFARAY

PolyDouble::ClipResult PolyDouble::planeClip(double pos, const ClipPlane &clip_plane, const PolyDouble &poly)
{
	const int poly_num_vertices = poly.numVertices();
	if(poly_num_vertices < 3)
	{
		Y_WARNING << "Polygon clip: polygon with only " << poly.numVertices() << " vertices (less than 3), poly is 'degenerated'!" << YENDL;
		return ClipResult::FatalError;
	}
	else if(poly_num_vertices > 10)
	{
		Y_WARNING << "Polygon clip: polygon with " << poly.numVertices() << " vertices (more than maximum 10), poly is limited to 10 vertices/edges, something is wrong!" << YENDL;
		return ClipResult::FatalError;
	}
	PolyDouble::ClipResult clip_result;
	const int curr_axis = clip_plane.axis_;
	const int next_axis = Axis::next(curr_axis);
	const int prev_axis = Axis::prev(curr_axis);
	for(int vert = 0; vert < poly_num_vertices; vert++) // for each poly edge
	{
		const int next_vert = (vert + 1) % poly_num_vertices;
		const Vec3Double &vert_1 = poly[vert];
		const Vec3Double &vert_2 = poly[next_vert];
		const bool vert_1_inside = (clip_plane.pos_ == ClipPlane::Pos::Lower) ? (vert_1[curr_axis] >= pos) : (vert_1[curr_axis] <= pos);
		if(vert_1_inside)   // vert_1 inside
		{
			const bool vert_2_inside = (clip_plane.pos_ == ClipPlane::Pos::Lower) ? (vert_2[curr_axis] >= pos) : (vert_2[curr_axis] <= pos);
			if(vert_2_inside) //both "inside"
			{
				// copy vert_2 to new poly
				clip_result.poly_.addVertex(vert_2);
			}
			else
			{
				// clip line, add intersection to new poly
				const double t = (pos - vert_1[curr_axis]) / (vert_2[curr_axis] - vert_1[curr_axis]);
				Vec3Double vert_new;
				vert_new[curr_axis] = pos;
				vert_new[next_axis] = vert_1[next_axis] + t * (vert_2[next_axis] - vert_1[next_axis]);
				vert_new[prev_axis] = vert_1[prev_axis] + t * (vert_2[prev_axis] - vert_1[prev_axis]);
				clip_result.poly_.addVertex(vert_new);
			}
		}
		else //vert_1 < b_min -> outside
		{
			const bool vert_2_inside = (clip_plane.pos_ == ClipPlane::Pos::Lower) ? (vert_2[curr_axis] > pos) : (vert_2[curr_axis] < pos);
			if(vert_2_inside) //vert_2 inside, add vert_new and vert_2
			{
				const double t = (pos - vert_2[curr_axis]) / (vert_1[curr_axis] - vert_2[curr_axis]);
				Vec3Double vert_new;
				vert_new[curr_axis] = pos;
				vert_new[next_axis] = vert_2[next_axis] + t * (vert_1[next_axis] - vert_2[next_axis]);
				vert_new[prev_axis] = vert_2[prev_axis] + t * (vert_1[prev_axis] - vert_2[prev_axis]);
				clip_result.poly_.addVertex(vert_new);
				clip_result.poly_.addVertex(vert_2);
			}
			else if(vert_2[curr_axis] == pos) //vert_2 and vert_new are identical, only add vert_2
			{
				clip_result.poly_.addVertex(vert_2);
			}
		}
		//else: both out, do nothing.
	} //for all edges
	const int clipped_poly_num_vertices = clip_result.poly_.numVertices();
	if(clipped_poly_num_vertices == 0) return ClipResult::NoOverlapDisappeared;
	else if(clipped_poly_num_vertices > 10)
	{
		Y_WARNING << "Polygon clip: polygon with " << poly_num_vertices << " vertices, after clipping has " << clipped_poly_num_vertices << " vertices, that cannot happen as PolyDouble class is limited to a maximum of 10 edges/vertices, up to a quad plus up to a maximum 6 extra vertices (one for each clipping plane), something is wrong!" << YENDL;
		return ClipResult::FatalError;
	}
	else if(clipped_poly_num_vertices < 3)
	{
		Y_WARNING << "Polygon clip: polygon with " << poly_num_vertices << " vertices, after clipping has only " << clipped_poly_num_vertices << " vertices, clip is 'degenerated'!" << YENDL;
		return ClipResult::DegeneratedLessThan3Edges;
	}
	clip_result.clip_result_code_ = ClipResult::Correct;
	return clip_result;
}

Bound PolyDouble::getBound(const PolyDouble &poly)
{
	Vec3Double a, g;
	for(int i = 0; i < 3; ++i) a[i] = g[i] = poly[0][i];
	const int poly_num_vertices = poly.numVertices();
	for(int i = 1; i < poly_num_vertices; ++i)
	{
		for(int j = 0; j < 3; ++j)
		{
			a[j] = std::min(a[j], poly[i][j]);
			g[j] = std::max(g[j], poly[i][j]);
		}
	}
	Bound bound;
	for(int i = 0; i < 3; ++i)
	{
		bound.a_[i] = a[i];
		bound.g_[i] = g[i];
	}
	return bound;
}

PolyDouble::ClipResultWithBound PolyDouble::planeClipWithBound(double pos, const ClipPlane &clip_plane, const PolyDouble &poly)
{
	ClipResultWithBound clip_result = planeClip(pos, clip_plane, poly);
	clip_result.box_ = getBound(clip_result.poly_);
	clip_result.clip_result_code_ = ClipResult::Correct;
	return clip_result;
}

/*! function to clip a polygon against an axis aligned bounding box and return new bound
	\param box the AABB of the clipped polygon
	\return ClipResult::Correct: polygon was clipped successfully
			ClipResult::NoOverlapDisappeared: polygon didn't overlap the bound at all => disappeared
			ClipResult::FatalError: fatal error occured
			ClipResult::DegeneratedLessThan3Edges: resulting polygon degenerated to less than 3 edges
*/
PolyDouble::ClipResultWithBound PolyDouble::boxClip(const Vec3Double &b_min, const Vec3Double &b_max, const PolyDouble &poly)
{
	ClipResultWithBound clip_result;
	clip_result.poly_ = poly;
	//for each axis
	for(int axis = 0; axis < 3; ++axis)
	{
		clip_result = planeClip(b_min[axis], {axis, ClipPlane::Pos::Lower}, clip_result.poly_);
		if(clip_result.clip_result_code_ != ClipResult::Correct) return {clip_result.clip_result_code_ };
		clip_result = planeClip(b_max[axis], {axis, ClipPlane::Pos::Upper}, clip_result.poly_);
		if(clip_result.clip_result_code_ != ClipResult::Correct) return {clip_result.clip_result_code_ };
	}
	clip_result.box_ = getBound(clip_result.poly_);
	return clip_result;
}

std::string PolyDouble::print() const
{
	std::string result = "{ ";
	for(size_t vert = 0; vert < size_; ++vert)
	{
		result += "[" + std::to_string(vert) + "]=" + vertices_[vert].print() + ", ";
	}
	result += " }";
	return result;
}

END_YAFARAY