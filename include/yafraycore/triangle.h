#pragma once

#ifndef Y_TRIANGLE_H
#define Y_TRIANGLE_H

#include <yafray_constants.h>
#include <yafray_config.h>
#include "meshtypes.h"
#include <core_api/primitive.h>

__BEGIN_YAFRAY

#define Y_MIN3(a,b,c) ( ((a)>(b)) ? ( ((b)>(c))?(c):(b)):( ((a)>(c))?(c):(a)) )
#define Y_MAX3(a,b,c) ( ((a)<(b)) ? ( ((b)>(c))?(b):(c)):( ((a)>(c))?(a):(c)) )

// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
int triBoxOverlap(double boxcenter[3],double boxhalfsize[3],double triverts[3][3]);


/*! non-inherited triangle, so no virtual functions to allow inlining
	othwise totally identically to vTriangle_t (when it actually ever
	makes it into release)
*/
class YAFRAYCORE_EXPORT triangle_t
{
	friend class scene_t;
	friend class triangleObject_t;
	friend class triangleInstance_t;

	public:
		triangle_t(): pa(-1), pb(-1), pc(-1), na(-1), nb(-1), nc(-1), mesh(nullptr), intersectionBiasFactor(0.f), edge1(0.f), edge2(0.f) { /* Empty */ }
		virtual ~triangle_t() { }
        triangle_t(int ia, int ib, int ic, triangleObject_t* m): pa(ia), pb(ib), pc(ic), na(-1), nb(-1), nc(-1), mesh(m), intersectionBiasFactor(0.f), edge1(0.f), edge2(0.f) {  updateIntersectionCachedValues(); }
		virtual bool intersect(const ray_t &ray, float *t, intersectData_t &data) const;
		virtual bound_t getBound() const;
		virtual bool intersectsBound(exBound_t &eb) const;
		virtual bool clippingSupport() const{ return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const;
		virtual const material_t* getMaterial() const { return material; }
		virtual void getSurface(surfacePoint_t &sp, const point3d_t &hit, intersectData_t &data) const;
		virtual float surfaceArea() const;
		virtual void sample(float s1, float s2, point3d_t &p, vector3d_t &n) const;

		virtual vector3d_t getNormal() const{ return vector3d_t(normal); }
		void setVertexIndices(int a, int b, int c){ pa=a, pb=b, pc=c; updateIntersectionCachedValues(); }
		void setMaterial(const material_t *m) { material = m; }
		void setNormals(int a, int b, int c){ na=a, nb=b, nc=c; }
		virtual void recNormal();
		size_t getIndex() const { return selfIndex; }
        bool operator == (triangle_t const& a) const
        {
            return selfIndex == a.selfIndex;
        }
        friend std::ostream & operator << (std::ostream &out,const triangle_t &t)
        {
            out << "[ idx = " << t.selfIndex << " (" << t.pa << "," << t.pb << "," << t.pc << ")]";
            return out;
        }
        virtual const triangleObject_t* getMesh() const { return mesh; }
        virtual void updateIntersectionCachedValues();

	private:
		int pa, pb, pc; //!< indices in point array, referenced in mesh.
		int na, nb, nc; //!< indices in normal array, if mesh is smoothed.
		const material_t* material;
		vector3d_t normal; //!< the geometric normal
        const triangleObject_t* mesh;
		size_t selfIndex;
		float intersectionBiasFactor;	//!< Intersection Bias factor based on longest edge to reduce 
		vector3d_t edge1, edge2;
};

class YAFRAYCORE_EXPORT triangleInstance_t: public triangle_t
{
	friend class scene_t;
	friend class triangleObjectInstance_t;

	public:
		triangleInstance_t(): mBase(nullptr), mesh(nullptr) { }
        triangleInstance_t(triangle_t* base, triangleObjectInstance_t* m): mBase(base), mesh(m) { updateIntersectionCachedValues();}
		virtual bool intersect(const ray_t &ray, float *t, intersectData_t &data) const;
		virtual bound_t getBound() const;
		virtual bool intersectsBound(exBound_t &eb) const;
		virtual bool clippingSupport() const { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const;
		virtual const material_t* getMaterial() const { return mBase->getMaterial(); }
		virtual void getSurface(surfacePoint_t &sp, const point3d_t &hit, intersectData_t &data) const;
		virtual float surfaceArea() const;
		virtual void sample(float s1, float s2, point3d_t &p, vector3d_t &n) const;

		virtual vector3d_t getNormal() const;
		virtual void recNormal() { /* Empty */ };
		virtual void updateIntersectionCachedValues();

	private:
        const triangle_t* mBase;
        const triangleObjectInstance_t* mesh;
};

/*! inherited triangle, so has virtual functions; connected to meshObject_t;
	otherwise identical to triangle_t
*/

class YAFRAYCORE_EXPORT vTriangle_t: public primitive_t
{
	friend class scene_t;
	public:
		vTriangle_t(){};
		vTriangle_t(int ia, int ib, int ic, meshObject_t* m): pa(ia), pb(ib), pc(ic),
					na(-1), nb(-1), nc(-1), mesh(m){ /*recNormal();*/ };
		virtual bool intersect(const ray_t &ray, float *t, intersectData_t &data) const;
		virtual bound_t getBound() const;
		virtual bool intersectsBound(exBound_t &eb) const;
		virtual bool clippingSupport() const { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const;
		virtual const material_t* getMaterial() const { return material; }
		virtual void getSurface(surfacePoint_t &sp, const point3d_t &hit, intersectData_t &data) const;

		// following are methods which are not part of primitive interface:
		void setMaterial(const material_t *m) { material = m; }
		void setNormals(int a, int b, int c){ na=a, nb=b, nc=c; }
		vector3d_t getNormal(){ return vector3d_t(normal); }
		float surfaceArea() const;
		void sample(float s1, float s2, point3d_t &p, vector3d_t &n) const;
		void recNormal();

	protected:
		int pa, pb, pc; //!< indices in point array, referenced in mesh.
		int na, nb, nc; //!< indices in normal array, if mesh is smoothed.
		normal_t normal; //!< the geometric normal
		const material_t* material;
		const meshObject_t* mesh;
};

/*! a triangle supporting time based deformation described by a quadratic bezier spline */
class YAFRAYCORE_EXPORT bsTriangle_t: public primitive_t
{
	friend class scene_t;
	public:
		bsTriangle_t(){};
		bsTriangle_t(int ia, int ib, int ic, meshObject_t* m): pa(ia), pb(ib), pc(ic),
					na(-1), nb(-1), nc(-1), mesh(m){ };
		virtual bool intersect(const ray_t &ray, float *t, intersectData_t &data) const;
		virtual bound_t getBound() const;
		//virtual bool intersectsBound(exBound_t &eb) const;
		// return: false:=doesn't overlap bound; true:=valid clip exists
		//virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const;
		virtual const material_t* getMaterial() const { return material; }
		virtual void getSurface(surfacePoint_t &sp, const point3d_t &hit, intersectData_t &data) const;

		// following are methods which are not part of primitive interface:
		void setMaterial(const material_t *m) { material = m; }
		void setNormals(int a, int b, int c){ na=a, nb=b, nc=c; }
		//float surfaceArea() const;
		//void sample(float s1, float s2, point3d_t &p, vector3d_t &n) const;

	protected:
		int pa, pb, pc; //!< indices in point array, referenced in mesh.
		int na, nb, nc; //!< indices in normal array, if mesh is smoothed.
		//normal_t normal; //!< the geometric normal
		const material_t* material;
		const meshObject_t* mesh;
};

	inline void triangle_t::updateIntersectionCachedValues()
	{
		point3d_t const& a = mesh->getVertex(pa);
		point3d_t const& b = mesh->getVertex(pb);
		point3d_t const& c = mesh->getVertex(pc);

		edge1 = b - a;
		edge2 = c - a;

		intersectionBiasFactor = 0.1f * MIN_RAYDIST * std::max(edge1.length(), edge2.length());
	}


	inline void triangleInstance_t::updateIntersectionCachedValues()
	{
		point3d_t const& a = mesh->getVertex(mBase->pa);
		point3d_t const& b = mesh->getVertex(mBase->pb);
		point3d_t const& c = mesh->getVertex(mBase->pc);

		edge1 = b - a;
		edge2 = c - a;

		intersectionBiasFactor = 0.1f * MIN_RAYDIST * std::max(edge1.length(), edge2.length());
	}


	inline bool triangle_t::intersect(const ray_t &ray, float *t, intersectData_t &data) const
	{
		// Tomas Möller and Ben Trumbore ray intersection scheme
		// Getting the barycentric coordinates of the hit point
		// const point3d_t &a=mesh->points[pa], &b=mesh->points[pb], &c=mesh->points[pc];

		point3d_t const& a = mesh->getVertex(pa);

		vector3d_t pvec = ray.dir ^ edge2;
		float det = edge1 * pvec;

		float epsilon = intersectionBiasFactor;

		if(det > -epsilon && det < epsilon) return false;

		float inv_det = 1.f / det;
		vector3d_t tvec = ray.from - a;
		float u = (tvec*pvec) * inv_det;

		if (u < 0.f || u > 1.f) return false;

		vector3d_t qvec = tvec^edge1;
		float v = (ray.dir*qvec) * inv_det;

		if ((v<0.f) || ((u+v)>1.f) ) return false;

		*t = edge2 * qvec * inv_det;

		if(*t < epsilon) return false;

		data.b1 = u;
		data.b2 = v;
		data.b0 = 1 - u - v;
		data.edge1 = &edge1;
		data.edge2 = &edge2;
		return true;
	}

	inline bound_t triangle_t::getBound() const
	{
		point3d_t const& a = mesh->getVertex(pa);
		point3d_t const& b = mesh->getVertex(pb);
		point3d_t const& c = mesh->getVertex(pc);

		point3d_t l, h;
		l.x = Y_MIN3(a.x, b.x, c.x);
		l.y = Y_MIN3(a.y, b.y, c.y);
		l.z = Y_MIN3(a.z, b.z, c.z);
		h.x = Y_MAX3(a.x, b.x, c.x);
		h.y = Y_MAX3(a.y, b.y, c.y);
		h.z = Y_MAX3(a.z, b.z, c.z);
		return bound_t(l, h);
	}

	inline bool triangle_t::intersectsBound(exBound_t &eb) const
	{
		double tPoints[3][3];

		point3d_t const& a = mesh->getVertex(pa);
		point3d_t const& b = mesh->getVertex(pb);
		point3d_t const& c = mesh->getVertex(pc);

		for(int j=0; j<3; ++j)
		{
			tPoints[0][j] = a[j];
			tPoints[1][j] = b[j];
			tPoints[2][j] = c[j];
		}
		// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
		return triBoxOverlap(eb.center, eb.halfSize, tPoints);
	}

	inline void triangle_t::recNormal()
	{
		point3d_t const& a = mesh->getVertex(pa);
		point3d_t const& b = mesh->getVertex(pb);
		point3d_t const& c = mesh->getVertex(pc);

		normal = ((b-a)^(c-a)).normalize();
	}

// triangleInstance_t inlined functions

	inline bool triangleInstance_t::intersect(const ray_t &ray, float *t, intersectData_t &data) const
	{
		// Tomas Möller and Ben Trumbore ray intersection scheme
		// Getting the barycentric coordinates of the hit point
		point3d_t const& a = mesh->getVertex(mBase->pa);

		vector3d_t pvec = ray.dir ^ edge2;
		float det = edge1 * pvec;

		float epsilon = intersectionBiasFactor;

		if(det > -epsilon && det < epsilon) return false;

		float inv_det = 1.f / det;
		vector3d_t tvec = ray.from - a;
		float u = (tvec*pvec) * inv_det;

		if (u < 0.f || u > 1.f) return false;

		vector3d_t qvec = tvec^edge1;
		float v = (ray.dir*qvec) * inv_det;

		if ((v<0.f) || ((u+v)>1.f) ) return false;

		*t = edge2 * qvec * inv_det;

		if(*t < epsilon) return false;

		data.b1 = u;
		data.b2 = v;
		data.b0 = 1 - u - v;
		data.edge1 = &edge1;
		data.edge2 = &edge2;
		return true;
	}

	inline bound_t triangleInstance_t::getBound() const
	{
		point3d_t const& a = mesh->getVertex(mBase->pa);
		point3d_t const& b = mesh->getVertex(mBase->pb);
		point3d_t const& c = mesh->getVertex(mBase->pc);

		point3d_t l, h;
		l.x = Y_MIN3(a.x, b.x, c.x);
		l.y = Y_MIN3(a.y, b.y, c.y);
		l.z = Y_MIN3(a.z, b.z, c.z);
		h.x = Y_MAX3(a.x, b.x, c.x);
		h.y = Y_MAX3(a.y, b.y, c.y);
		h.z = Y_MAX3(a.z, b.z, c.z);
		return bound_t(l, h);
	}

	inline bool triangleInstance_t::intersectsBound(exBound_t &eb) const
	{
		double tPoints[3][3];

		point3d_t const& a = mesh->getVertex(mBase->pa);
		point3d_t const& b = mesh->getVertex(mBase->pb);
		point3d_t const& c = mesh->getVertex(mBase->pc);

		for(int j=0; j<3; ++j)
		{
			tPoints[0][j] = a[j];
			tPoints[1][j] = b[j];
			tPoints[2][j] = c[j];
		}
		// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
		return triBoxOverlap(eb.center, eb.halfSize, tPoints);
	}

	inline vector3d_t triangleInstance_t::getNormal() const
	{
		return vector3d_t(mesh->objToWorld * mBase->normal).normalize();
	}


__END_YAFRAY

#endif // Y_TRIANGLE_H
