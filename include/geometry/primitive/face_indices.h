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

#ifndef LIBYAFARAY_FACE_INDICES_H
#define LIBYAFARAY_FACE_INDICES_H

#include "geometry/primitive/vertex_indices.h"

namespace yafaray {

template <typename IndexType>
class FaceIndices
{
	public:
		FaceIndices() = default;
		FaceIndices(const FaceIndices<IndexType> &face_indices) = default;
		FaceIndices(FaceIndices &&face_indices) noexcept = default;
		explicit FaceIndices(const std::array<VertexIndices<IndexType>, 4> &vertices_indices) : vertices_indices_{vertices_indices} {};
		explicit FaceIndices(std::array<VertexIndices<IndexType>, 4> &&vertices_indices) : vertices_indices_{std::move(vertices_indices)} {};
		bool hasUv() const { return vertices_indices_[0].uv_ != math::invalid<IndexType>; }
		bool isQuad() const { return vertices_indices_[3].vertex_ != math::invalid<IndexType>; }
		int numVertices() const { return isQuad() ? 4 : 3; }
		const VertexIndices<IndexType> &operator[](IndexType index) const { if(index >= vertices_indices_.size()) index = 0; return vertices_indices_[index]; }
		VertexIndices<IndexType> &operator[](IndexType index) { if(index >= vertices_indices_.size()) index = 0; return vertices_indices_[index]; }
		typename std::array<VertexIndices<IndexType>, 4>::iterator begin() { return vertices_indices_.begin(); }
		typename std::array<VertexIndices<IndexType>, 4>::iterator end() { return vertices_indices_.end(); }
		typename std::array<VertexIndices<IndexType>, 4>::const_iterator begin() const { return vertices_indices_.begin(); }
		typename std::array<VertexIndices<IndexType>, 4>::const_iterator end() const { return vertices_indices_.end(); }

	private:
		std::array<VertexIndices<IndexType>, 4> vertices_indices_;
};

} //namespace yafaray

#endif //LIBYAFARAY_FACE_INDICES_H
