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

#ifndef LIBYAFARAY_VERTEX_INDICES_H
#define LIBYAFARAY_VERTEX_INDICES_H

#include "math/math.h"

namespace yafaray {

template <typename IndexType>
struct VertexIndices
{
	IndexType vertex_{math::invalid<IndexType>}; //!< index in point array, referenced in mesh.
	IndexType normal_{math::invalid<IndexType>}; //!< index in normal array, if mesh is smoothed.//!< indices in normal array, if mesh is smoothed.
	IndexType uv_{math::invalid<IndexType>}; //!< index in uv array, if mesh has explicit uv.
};

} //namespace yafaray

#endif //LIBYAFARAY_VERTEX_INDICES_H
