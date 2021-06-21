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

#ifndef YAFARAY_MEMORY_H
#define YAFARAY_MEMORY_H

#include "yafaray_common.h"
#include <memory>

BEGIN_YAFARAY

/*! This CustomDeleter structure can be used as custom deleter in any unique_ptr object whose class has the setAutoDelete(bool value) and isAutoDeleted() functions.
 * This allows to set the unique_ptr to either:
 *   - If isAutoDeleted() is true, default unique_ptr behavior: automatically be deleted when it gets out of scope or the containing object is destroyed.
 *   - If isAutoDeleted() is false, only for libYafaRay clients owning the object, the object will not be deleted when unique_ptr gets out of the scope or containing object destroyed, even surviving the end of the libYafaRay instance itself. In that case it's the client's responsibility to delete the object.
 *   - getName() function is also required in the class to get object name (if any)
 */
template <typename T>
struct CustomDeleter
{
	void operator()(T *object);
};

template <typename T, typename Base = T> using UniquePtr_t = std::unique_ptr<T, CustomDeleter<Base>>; //!< Customized std::unique_ptr with optional custom deleter. If no custom deleter specified, it becomes a regular std::unique_ptr

END_YAFARAY

#endif //YAFARAY_MEMORY_H
