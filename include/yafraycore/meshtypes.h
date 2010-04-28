
#ifndef Y_MESHTYPES_H
#define Y_MESHTYPES_H

#include <core_api/object3d.h>
#include <yafraycore/triangle.h>


__BEGIN_YAFRAY


struct uv_t
{
	uv_t(GFLOAT _u, GFLOAT _v): u(_u), v(_v) {};
	GFLOAT u, v;
};

class triangle_t;
class vTriangle_t;

/*!	meshObject_t holds various polygonal primitives
*/

class YAFRAYCORE_EXPORT meshObject_t: public object3d_t
{
	friend class vTriangle_t;
	friend class bsTriangle_t;
	friend class scene_t;
	public:
		meshObject_t(int ntris, bool hasUV=false, bool hasOrco=false);
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		int numPrimitives() const { return triangles.size() + s_triangles.size(); }
		int getPrimitives(const primitive_t **prims) const;
		
		primitive_t* addTriangle(const vTriangle_t &t);
		primitive_t* addBsTriangle(const bsTriangle_t &t);
		
		void setContext(std::vector<point3d_t>::iterator p, std::vector<normal_t>::iterator n);
		void setLight(const light_t *l){ light=l; }
		void finish();
	protected:
		std::vector<vTriangle_t> triangles;
		std::vector<bsTriangle_t> s_triangles;
		std::vector<point3d_t>::iterator points;
		std::vector<normal_t>::iterator normals;
		std::vector<int> uv_offsets;
		std::vector<uv_t> uv_values;
		bool has_orco;
		bool has_uv;
		bool has_vcol;
		bool is_smooth;
		const light_t *light;
		const matrix4x4_t world2obj; //!< transformation from world to object coordinates
};

/*!	This is a special version of meshObject_t!
	The only difference is that it returns a triangle_t instead of vTriangle_t,
	see declaration if triangle_t for more details!
*/

class YAFRAYCORE_EXPORT triangleObject_t: public object3d_t
{
	friend class triangle_t;
	friend class scene_t;
	public:
		triangleObject_t(int ntris, bool hasUV=false, bool hasOrco=false);
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const { return triangles.size(); }
		/*! cannot return primitive_t...yet */
		virtual int getPrimitives(const primitive_t **prims) const{ return 0; }
		int getPrimitives(const triangle_t **prims);
		
		triangle_t* addTriangle(const triangle_t &t);
		
		void setContext(std::vector<point3d_t>::iterator p, std::vector<normal_t>::iterator n);
		void finish();
	protected:
		std::vector<triangle_t> triangles;
		std::vector<point3d_t>::iterator points;
		std::vector<normal_t>::iterator normals;
		std::vector<int> uv_offsets;
		std::vector<uv_t> uv_values;
		bool has_orco;
		bool has_uv;
		bool has_vcol;
		bool is_smooth;
		const matrix4x4_t world2obj; //!< transformation from world to object coordinates
};

#include <yafraycore/triangle_inline.h>

__END_YAFRAY


#endif //Y_MESHTYPES_H

