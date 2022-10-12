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
		virtual void setName(const std::string &name) = 0;
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const = 0;
		/*! write the primitive pointers to the given array
			\return number of written primitives */
		virtual std::vector<const Primitive *> getPrimitives() const = 0;
		virtual void setVisibility(Visibility visibility) = 0;
		/*! Indicates that this object should be used as base object for instances */
		virtual void useAsBaseObject(bool v) = 0;
		/*! Returns if this object should be used for rendering and/or shadows. */
		virtual Visibility getVisibility() const = 0;
		/*! Returns if this object is used as base object for instances. */
		virtual bool isBaseObject() const = 0;
		virtual void setIndexAuto(unsigned int new_obj_index) = 0;
		virtual unsigned int getIndex() const = 0;
		virtual Rgb getIndexAutoColor() const = 0;
		virtual unsigned int getIndexAuto() const = 0;
		virtual const Light *getLight() const = 0;
		/*! set a light source to be associated with this object */
		virtual void setLight(const Light *light) = 0;
		virtual bool calculateObject(const std::unique_ptr<const Material> *material) = 0;
		bool calculateObject() { return calculateObject(nullptr); }

		/* Mesh-related interface functions below, only for Mesh objects */
		virtual int lastVertexId(int time_step) const { return -1; }
		virtual void addPoint(Point3f &&p, int time_step) { }
		virtual void addOrcoPoint(Point3f &&p, int time_step) { }
		virtual void addVertexNormal(Vec3f &&n, int time_step) { }
		virtual void addFace(std::vector<int> &&vertices, std::vector<int> &&vertices_uv, const std::unique_ptr<const Material> *material) { }
		virtual int addUvValue(Uv<float> &&uv) { return -1; }
		virtual bool hasVerticesNormals(int time_step) const { return false; }
		virtual int numVerticesNormals(int time_step) const { return 0; }
		virtual int numVertices(int time_step) const { return 0; }
		virtual void setSmooth(bool smooth) { }
		virtual bool smoothVerticesNormals(Logger &logger, float angle) { return false; }
		virtual bool hasMotionBlur() const { return false; }
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_H
