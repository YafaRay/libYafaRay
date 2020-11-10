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

#ifndef YAFARAY_OBJECT_TRIANGLE_INSTANCE_H
#define YAFARAY_OBJECT_TRIANGLE_INSTANCE_H

#include "geometry/object_triangle.h"
#include "geometry/triangle_instance.h"
#include "geometry/matrix4.h"
#include <vector>

BEGIN_YAFARAY

class TriangleInstance;

class TriangleObjectInstance final: public TriangleObject
{
	public:
		TriangleObjectInstance(const TriangleObject *base, Matrix4 obj_2_world);
		const TriangleObject *getBaseTriangleObject() const { return triangle_object_; }
		const Matrix4 &getObjToWorldMatrix() const { return obj_to_world_; }
		virtual Point3 getVertex(int index) const override { return obj_to_world_ * triangle_object_->getPoints()[index]; }
		virtual Vec3 getVertexNormal(int index) const override { return obj_to_world_ * triangle_object_->getNormals()[index]; }
		const std::vector<TriangleInstance> &getTriangleInstances() const { return triangle_instances_; }
		virtual const Light *getLight() const override { if(triangle_object_) return triangle_object_->getLight(); else return nullptr; }
		virtual bool hasOrco() const override { return triangle_object_->hasOrco(); }
		virtual bool hasUv() const override { return triangle_object_->hasUv(); }
		virtual bool isSmooth() const override { return triangle_object_->isSmooth(); }
		virtual bool hasNormalsExported() const override { return triangle_object_->hasNormalsExported(); }

	private:
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const override { return triangle_instances_.size(); }
		virtual int getPrimitives(const Triangle **prims) const override;
		virtual void finish() override;

		std::vector<TriangleInstance> triangle_instances_;
		Matrix4 obj_to_world_;
		const TriangleObject *triangle_object_ = nullptr;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_TRIANGLE_INSTANCE_H
