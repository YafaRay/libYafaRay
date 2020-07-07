#pragma once

#ifndef Y_OBJECT3D_H
#define Y_OBJECT3D_H

#include <yafray_constants.h>
#include "color.h"

__BEGIN_YAFRAY

class light_t;
class primitive_t;
class surfacePoint_t;
class point3d_t;
class vector3d_t;

class YAFRAYCORE_EXPORT object3d_t
{
	public:
        object3d_t(): light(nullptr), visible(true), is_base_mesh(false), objectIndex(0.f)
        {
			objectIndexAuto++;
			srand(objectIndexAuto);		
			float R,G,B;
			do
			{
				R = (float) (rand() % 8) / 8.f;
				G = (float) (rand() % 8) / 8.f;
				B = (float) (rand() % 8) / 8.f;
			}
			while (R+G+B < 0.5f);
			objectIndexAutoColor = color_t(R,G,B);
            objectIndexAutoNumber = objectIndexAuto;
		}
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const = 0;
		/*! write the primitive pointers to the given array
			\return number of written primitives */
		virtual int getPrimitives(const primitive_t **prims) const { return 0; }
		/*! set a light source to be associated with this object */
		virtual void setLight(const light_t *l){ light=l; }
		/*! query whether object surface can be sampled right now */
		virtual bool canSample() { return false; }
		/*! try to enable sampling (may require additional memory and preprocessing time, if supported) */
		virtual bool enableSampling() { return false; }
		/*! sample object surface */
		virtual void sample(float s1, float s2, point3d_t &p, vector3d_t &n) const {};
		/*! Sets the object visibility to the renderer (is added or not to the kdtree) */
		void setVisibility(bool v) { visible = v; }
		/*! Indicates that this object should be used as base object for instances */
		void useAsBaseObject(bool v) { is_base_mesh = v; }
		/*! Returns if this object should be used for rendering. */
		bool isVisible() const { return visible; }
		/*! Returns if this object is used as base object for instances. */
		bool isBaseObject() const { return is_base_mesh; }
		virtual ~object3d_t(){ resetObjectIndex(); }
        void setObjectIndex(const float &newObjIndex)
        {
			objectIndex = newObjIndex;
			if(highestObjectIndex < objectIndex) highestObjectIndex = objectIndex;
		}
        void resetObjectIndex() { highestObjectIndex = 1.f; objectIndexAuto = 0; }
        void setObjectIndex(const int &newObjIndex) { setObjectIndex((float) newObjIndex); }
        float getAbsObjectIndex() const { return objectIndex; }
        float getNormObjectIndex() const { return (objectIndex / highestObjectIndex); }
        color_t getAbsObjectIndexColor() const
		{
			return color_t(objectIndex);
		}
		color_t getNormObjectIndexColor() const
		{
			float normalizedObjectIndex = getNormObjectIndex();
			return color_t(normalizedObjectIndex);
		}
        color_t getAutoObjectIndexColor() const
		{
			return objectIndexAutoColor;
		}
        color_t getAutoObjectIndexNumber() const
		{
			return objectIndexAutoNumber;
		}

	protected:
		const light_t *light;
		bool visible; //!< toggle whether geometry is visible or only guidance for other stuff
        bool is_base_mesh;
		float objectIndex;	//!< Object Index for the object-index render pass
		static unsigned int objectIndexAuto;	//!< Object Index automatically generated for the object-index-auto render pass
		color_t objectIndexAutoColor;	//!< Object Index color automatically generated for the object-index-auto color render pass
        color_t objectIndexAutoNumber = 0.f;	//!< Object Index number automatically generated for the object-index-auto-abs numeric render pass
		static float highestObjectIndex;	//!< Class shared variable containing the highest object index used for the Normalized Object Index pass.
};



/*! simple "container" to handle primitives as objects, for objects that
	consist of just one primitive like spheres etc. */
class primObject_t : public object3d_t
{
	public:
		primObject_t(primitive_t *p): prim(p) { };
		virtual int numPrimitives() const { return 1; }
		virtual int getPrimitives(const primitive_t **prims) const{ *prims = prim; return 1; }
	private:
		primitive_t *prim;
};

__END_YAFRAY

#endif // Y_OBJECT3D_H
