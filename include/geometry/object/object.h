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

#ifndef LIBYAFARAY_OBJECT_H
#define LIBYAFARAY_OBJECT_H

#include "color/color.h"
#include "common/visibility.h"
#include "common/logger.h"
#include <memory>
#include <vector>

namespace yafaray {

class Light;
class Primitive;
class SurfacePoint;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;
class ParamMap;
class Scene;
template <typename T, size_t N> class SquareMatrix;
typedef SquareMatrix<float, 4> Matrix4f;
class Material;
template <typename T> struct Uv;

class Object
{
	public:
		inline static std::string getClassName() { return "Object"; }
		virtual ~Object() = default;
		[[nodiscard]] virtual std::string getName() const = 0;
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_H
