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

#ifndef YAFARAY_POLY_DOUBLE_H
#define YAFARAY_POLY_DOUBLE_H

#include "geometry/vector_double.h"
#include <vector>
#include <array>
#include <common/logger.h>

BEGIN_YAFARAY

struct ClipPlane;
class Bound;

class PolyDouble
{
	public:
		size_t numVertices() const { return size_; }
		bool empty() const { return size_ == 0; }
		Vec3Double operator[](size_t index) const { return vertices_[index]; }
		void addVertex(const Vec3Double &vertex) { vertices_[size_++] = vertex; }
		std::string print() const;
		struct ClipResult;
		struct ClipResultWithBound;
		/* Sutherland-Hodgman triangle clipping */
		static PolyDouble::ClipResultWithBound planeClipWithBound(Logger &logger, double pos, const ClipPlane &clip_plane, const PolyDouble &poly);
		static PolyDouble::ClipResultWithBound boxClip(Logger &logger, const Vec3Double &b_max, const PolyDouble &poly, const Vec3Double &b_min);
	private:
		static PolyDouble::ClipResult planeClip(Logger &logger, double pos, const ClipPlane &clip_plane, const PolyDouble &poly);
		static Bound getBound(const PolyDouble &poly);
		std::array<Vec3Double, 10> vertices_; //Limited to triangles + 6 clipping planes cuts, or to quads + 6 clipping planes (total 10 edges/vertices max)
		size_t size_ = 0;
};

struct PolyDouble::ClipResult
{
	/*! enum class ClipResult: Result of clipping a triangle against an axis aligned bounding box
		ClipResult::Correct: triangle was clipped successfully
		ClipResult::NoOverlapDisappeared: triangle didn't overlap the bound at all => disappeared
		ClipResult::FatalError: fatal error occured (didn't ever happen to me :)
		ClipResult::DegeneratedLessThan3Edges: resulting polygon degenerated to less than 3 edges (never happened either)
	*/
	enum Code : int { Correct, NoOverlapDisappeared, FatalError, DegeneratedLessThan3Edges };
	explicit ClipResult(Code clip_result_code = Correct) : clip_result_code_(clip_result_code) { }
	Code clip_result_code_ = Correct;
	PolyDouble poly_;
};

struct PolyDouble::ClipResultWithBound : PolyDouble::ClipResult
{
	explicit ClipResultWithBound(Code clip_result_code = Correct);
	ClipResultWithBound(ClipResultWithBound &&clip_result_with_bound) = default;
	ClipResultWithBound& operator=(ClipResultWithBound&&) = default;
	explicit ClipResultWithBound(ClipResult &&clip_result);
	std::unique_ptr<Bound> box_;
};


END_YAFARAY

#endif //YAFARAY_POLY_DOUBLE_H
