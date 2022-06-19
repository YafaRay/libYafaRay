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
#include "common/logger.h"
#include "geometry/bound.h"
#include "geometry/clip_plane.h"
#include <array>
#include <memory>

namespace yafaray {

PolyDouble::ClipResultWithBound::ClipResultWithBound(Code clip_result_code) : PolyDouble::ClipResult(clip_result_code) { }
PolyDouble::ClipResultWithBound::ClipResultWithBound(ClipResult &&clip_result) : PolyDouble::ClipResult(std::move(clip_result)) { }

PolyDouble::ClipResult PolyDouble::planeClip(Logger &logger, double pos, const ClipPlane &clip_plane, const PolyDouble &poly)
{
	const size_t poly_num_vertices = poly.numVertices();
	if(poly_num_vertices < 3)
	{
		logger.logWarning("Polygon clip: polygon with only ", poly.numVertices(), " vertices (less than 3), poly is 'degenerated'!");
		return ClipResult(ClipResult::Code::FatalError);
	}
	else if(poly_num_vertices > 10)
	{
		logger.logWarning("Polygon clip: polygon with ", poly.numVertices(), " vertices (more than maximum 10), poly is limited to 10 vertices/edges, something is wrong!");
		return ClipResult(ClipResult::Code::FatalError);
	}
	PolyDouble::ClipResult clip_result;
	const Axis curr_axis = clip_plane.axis_;
	const Axis next_axis = axis::getNextSpatial(curr_axis);
	const Axis prev_axis = axis::getPrevSpatial(curr_axis);
	for(int vert = 0; vert < poly_num_vertices; vert++) // for each poly edge
	{
		const unsigned int next_vert = (vert + 1) % poly_num_vertices;
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
	const size_t clipped_poly_num_vertices = clip_result.poly_.numVertices();
	if(clipped_poly_num_vertices == 0) return ClipResult(ClipResult::Code::NoOverlapDisappeared);
	else if(clipped_poly_num_vertices > 10)
	{
		logger.logWarning("Polygon clip: polygon with ", poly_num_vertices, " vertices, after clipping has ", clipped_poly_num_vertices, " vertices, that cannot happen as PolyDouble class is limited to a maximum of 10 edges/vertices, up to a quad plus up to a maximum 6 extra vertices (one for each clipping plane), something is wrong!");
		return ClipResult(ClipResult::Code::FatalError);
	}
	else if(clipped_poly_num_vertices < 3)
	{
		logger.logWarning("Polygon clip: polygon with ", poly_num_vertices, " vertices, after clipping has only ", clipped_poly_num_vertices, " vertices, clip is 'degenerated'!");
		return ClipResult(ClipResult::Code::DegeneratedLessThan3Edges);
	}
	clip_result.clip_result_code_ = ClipResult::Code::Correct;
	return clip_result;
}

Bound PolyDouble::getBound(const PolyDouble &poly)
{
	Vec3Double a, g;
	for(const auto &axis : axis::spatial) a[axis] = g[axis] = poly[0][axis];
	for(size_t i = 1; i < poly.numVertices(); ++i)
	{
		for(const auto &axis : axis::spatial)
		{
			a[axis] = std::min(a[axis], poly[i][axis]);
			g[axis] = std::max(g[axis], poly[i][axis]);
		}
	}
	Bound bound;
	for(const auto &axis : axis::spatial)
	{
		bound.a_[axis] = static_cast<float>(a[axis]);
		bound.g_[axis] = static_cast<float>(g[axis]);
	}
	return bound;
}

PolyDouble::ClipResultWithBound PolyDouble::planeClipWithBound(Logger &logger, double pos, const ClipPlane &clip_plane, const PolyDouble &poly)
{
	ClipResultWithBound clip_result(planeClip(logger, pos, clip_plane, poly));
	if(clip_result.clip_result_code_ == ClipResult::Code::Correct)
	{
		clip_result.box_ = std::make_unique<Bound>();
		*clip_result.box_ = getBound(clip_result.poly_);
	}
	return clip_result;
}

/*! function to clip a polygon against an axis aligned bounding box and return new bound
	\param box the AABB of the clipped polygon
	\return ClipResult::Correct: polygon was clipped successfully
			ClipResult::NoOverlapDisappeared: polygon didn't overlap the bound at all => disappeared
			ClipResult::FatalError: fatal error occured
			ClipResult::DegeneratedLessThan3Edges: resulting polygon degenerated to less than 3 edges
*/
PolyDouble::ClipResultWithBound PolyDouble::boxClip(Logger &logger, const Vec3Double &b_max, const PolyDouble &poly, const Vec3Double &b_min)
{
	ClipResultWithBound clip_result;
	clip_result.poly_ = poly;
	//for each axis
	for(const auto &axis : axis::spatial)
	{
		clip_result = ClipResultWithBound(planeClip(logger, b_min[axis], {axis, ClipPlane::Pos::Lower}, clip_result.poly_));
		if(clip_result.clip_result_code_ != ClipResult::Code::Correct) return ClipResultWithBound(clip_result.clip_result_code_);
		clip_result = ClipResultWithBound(planeClip(logger, b_max[axis], {axis, ClipPlane::Pos::Upper}, clip_result.poly_));
		if(clip_result.clip_result_code_ != ClipResult::Code::Correct) return ClipResultWithBound(clip_result.clip_result_code_);
	}
	if(clip_result.clip_result_code_ == ClipResult::Code::Correct)
	{
		clip_result.box_ = std::make_unique<Bound>();
		*clip_result.box_ = getBound(clip_result.poly_);
	}
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

} //namespace yafaray