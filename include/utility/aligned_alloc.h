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

#ifndef YAFARAY_ALIGNED_ALLOC_H
#define YAFARAY_ALIGNED_ALLOC_H

#include "constants.h"
#include <cstdlib>

BEGIN_YAFARAY

inline void *alignedAlloc__(size_t alignment, size_t size)
{
#if defined(_WIN32)
	return _aligned_malloc(alignment, size);
#else //defined(_WIN32)
	return std::aligned_alloc(alignment, size);
#endif
}

inline void alignedFree__(void *ptr)
{
#if defined(_WIN32)
	return _aligned_free(ptr);
#else //defined(_WIN32)
	return std::free(ptr);
#endif
}

END_YAFARAY

#endif //YAFARAY_ALIGNED_ALLOC_H
