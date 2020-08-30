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

#ifndef YAFARAY_OBJECT3D_H
#define YAFARAY_OBJECT3D_H

#include <yafray_constants.h>
#include "color.h"

BEGIN_YAFRAY

class Light;
class Primitive;
class SurfacePoint;
class Point3;
class Vec3;

class YAFRAYCORE_EXPORT Object3D
{
	public:
		Object3D(): light_(nullptr), visible_(true), is_base_mesh_(false), object_index_(0.f)
		{
			object_index_auto_++;
			srand(object_index_auto_);
			float r, g, b;
			do
			{
				r = (float)(rand() % 8) / 8.f;
				g = (float)(rand() % 8) / 8.f;
				b = (float)(rand() % 8) / 8.f;
			}
			while(r + g + b < 0.5f);
			object_index_auto_color_ = Rgb(r, g, b);
			object_index_auto_number_ = object_index_auto_;
		}
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const = 0;
		/*! write the primitive pointers to the given array
			\return number of written primitives */
		virtual int getPrimitives(const Primitive **prims) const { return 0; }
		/*! set a light source to be associated with this object */
		virtual void setLight(const Light *l) { light_ = l; }
		/*! query whether object surface can be sampled right now */
		virtual bool canSample() { return false; }
		/*! try to enable sampling (may require additional memory and preprocessing time, if supported) */
		virtual bool enableSampling() { return false; }
		/*! sample object surface */
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const {};
		/*! Sets the object visibility to the renderer (is added or not to the kdtree) */
		void setVisibility(bool v) { visible_ = v; }
		/*! Indicates that this object should be used as base object for instances */
		void useAsBaseObject(bool v) { is_base_mesh_ = v; }
		/*! Returns if this object should be used for rendering. */
		bool isVisible() const { return visible_; }
		/*! Returns if this object is used as base object for instances. */
		bool isBaseObject() const { return is_base_mesh_; }
		virtual ~Object3D() { resetObjectIndex(); }
		void setObjectIndex(const float &new_obj_index)
		{
			object_index_ = new_obj_index;
			if(highest_object_index_ < object_index_) highest_object_index_ = object_index_;
		}
		void resetObjectIndex() { highest_object_index_ = 1.f; object_index_auto_ = 0; }
		void setObjectIndex(const int &new_obj_index) { setObjectIndex((float) new_obj_index); }
		float getAbsObjectIndex() const { return object_index_; }
		float getNormObjectIndex() const { return (object_index_ / highest_object_index_); }
		Rgb getAbsObjectIndexColor() const
		{
			return Rgb(object_index_);
		}
		Rgb getNormObjectIndexColor() const
		{
			float normalized_object_index = getNormObjectIndex();
			return Rgb(normalized_object_index);
		}
		Rgb getAutoObjectIndexColor() const
		{
			return object_index_auto_color_;
		}
		Rgb getAutoObjectIndexNumber() const
		{
			return object_index_auto_number_;
		}

	protected:
		const Light *light_;
		bool visible_; //!< toggle whether geometry is visible or only guidance for other stuff
		bool is_base_mesh_;
		float object_index_;	//!< Object Index for the object-index render pass
		static unsigned int object_index_auto_;	//!< Object Index automatically generated for the object-index-auto render pass
		Rgb object_index_auto_color_;	//!< Object Index color automatically generated for the object-index-auto color render pass
		Rgb object_index_auto_number_ = 0.f;	//!< Object Index number automatically generated for the object-index-auto-abs numeric render pass
		static float highest_object_index_;	//!< Class shared variable containing the highest object index used for the Normalized Object Index pass.
};



/*! simple "container" to handle primitives as objects, for objects that
	consist of just one primitive like spheres etc. */
class PrimObject : public Object3D
{
	public:
		PrimObject(Primitive *p): prim_(p) { };
		virtual int numPrimitives() const { return 1; }
		virtual int getPrimitives(const Primitive **prims) const { *prims = prim_; return 1; }
	private:
		Primitive *prim_;
};

END_YAFRAY

#endif // YAFARAY_OBJECT3D_H
