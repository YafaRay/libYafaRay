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

#ifndef YAFARAY_OBJECT_H
#define YAFARAY_OBJECT_H

#include "color/color.h"
#include "common/visibility.h"
#include "common/yafaray_common.h"
#include <common/logger.h>
#include <memory>
#include <vector>

BEGIN_YAFARAY

class Light;
class Primitive;
class SurfacePoint;
class Point3;
class Vec3;
class ParamMap;
class Scene;
class Matrix4;
class Material;
struct Uv;

class Object
{
	public:
		static Object *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		virtual ~Object() = default;
		virtual std::string getName() const = 0;
		virtual void setName(const std::string &name) = 0;
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const = 0;
		/*! write the primitive pointers to the given array
			\return number of written primitives */
		virtual std::vector<const Primitive *> getPrimitives() const = 0;
		/*! sample object surface */
		//virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n)  const = 0;
		/*! Sets the object visibility to the renderer (is added or not to the kdtree) */
		virtual void setVisibility(const Visibility &visibility) = 0;
		/*! Indicates that this object should be used as base object for instances */
		virtual void useAsBaseObject(bool v) = 0;
		/*! Returns if this object should be used for rendering and/or shadows. */
		virtual Visibility getVisibility() const = 0;
		/*! Returns if this object is used as base object for instances. */
		virtual bool isBaseObject() const = 0;
		virtual void setIndex(unsigned int new_obj_index) = 0;
		virtual void setIndexAuto(unsigned int new_obj_index) = 0;
		virtual unsigned int getIndex() const = 0;
		virtual unsigned int getIndexAuto() const = 0;
		virtual Rgb getIndexAutoColor() const = 0;
		virtual const Light *getLight() const = 0;
		/*! set a light source to be associated with this object */
		virtual void setLight(const Light *light) = 0;
		virtual bool calculateObject(const std::unique_ptr<const Material> *material) = 0;
		bool calculateObject() { return calculateObject(nullptr); }

		/* Mesh-related interface functions below, only for Mesh objects */
		virtual int lastVertexId(size_t time_step) const { return -1; }
		virtual void addPoint(const Point3 &p, size_t time_step) { }
		virtual void addOrcoPoint(const Point3 &p, size_t time_step) { }
		virtual void addVertexNormal(const Vec3 &n, size_t time_step) { }
		virtual void addFace(const std::vector<int> &vertices, const std::vector<int> &vertices_uv, const std::unique_ptr<const Material> *material) { }
		virtual int addUvValue(const Uv &uv) { return -1; }
		virtual bool hasVerticesNormals(size_t time_step) const { return false; }
		virtual int numVerticesNormals(size_t time_step) const { return 0; }
		virtual int numVertices(size_t time_step) const { return 0; }
		virtual void setSmooth(bool smooth) { }
		virtual bool smoothVerticesNormals(Logger &logger, float angle) { return false; }
		virtual bool hasMotionBlur() const { return false; }
};

END_YAFARAY

#endif //YAFARAY_OBJECT_H
