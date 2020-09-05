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

#ifndef YAFARAY_TRIANGLE_H
#define YAFARAY_TRIANGLE_H

#include "constants.h"
#include "yafaray_config.h"
#include "object_geom/object_geom_mesh.h"
#include "object_geom/primitive.h"
#include "common/bound.h"
#include "common/surface.h"

BEGIN_YAFARAY

template <class T>
constexpr T min3__(const T &a, const T &b, const T &c) { return std::min(a, std::min(b, c)); }

template <class T>
constexpr T max3__(const T &a, const T &b, const T &c) { return std::max(a, std::max(b, c)); }

// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
int triBoxOverlap__(double *boxcenter, double *boxhalfsize, double **triverts);


/*! non-inherited triangle, so no virtual functions to allow inlining
	othwise totally identically to vTriangle_t (when it actually ever
	makes it into release)
*/

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

class Triangle
{
		friend class Scene;
		friend class TriangleObject;
		friend class TriangleInstance;

	public:
		Triangle(): pa_(-1), pb_(-1), pc_(-1), na_(-1), nb_(-1), nc_(-1), mesh_(nullptr), intersection_bias_factor_(0.f), edge_1_(0.f), edge_2_(0.f) { /* Empty */ }
		virtual ~Triangle() { }
		Triangle(int ia, int ib, int ic, TriangleObject *m): pa_(ia), pb_(ib), pc_(ic), na_(-1), nb_(-1), nc_(-1), mesh_(m), intersection_bias_factor_(0.f), edge_1_(0.f), edge_2_(0.f) {  updateIntersectionCachedValues(); }
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const;
		virtual Bound getBound() const;
		virtual bool intersectsBound(ExBound &eb) const;
		virtual bool clippingSupport() const { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const;
		virtual const Material *getMaterial() const { return material_; }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const;
		virtual float surfaceArea() const;
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const;

		virtual Vec3 getNormal() const { return Vec3(normal_); }
		void setVertexIndices(int a, int b, int c) { pa_ = a, pb_ = b, pc_ = c; updateIntersectionCachedValues(); }
		void setMaterial(const Material *m) { material_ = m; }
		void setNormals(int a, int b, int c) { na_ = a, nb_ = b, nc_ = c; }
		virtual void recNormal();
		size_t getIndex() const { return self_index_; }
		bool operator == (Triangle const &a) const
		{
			return self_index_ == a.self_index_;
		}
		friend std::ostream &operator << (std::ostream &out, const Triangle &t)
		{
			out << "[ idx = " << t.self_index_ << " (" << t.pa_ << "," << t.pb_ << "," << t.pc_ << ")]";
			return out;
		}
		virtual const TriangleObject *getMesh() const { return mesh_; }
		virtual void updateIntersectionCachedValues();

	private:
		int pa_, pb_, pc_; //!< indices in point array, referenced in mesh.
		int na_, nb_, nc_; //!< indices in normal array, if mesh is smoothed.
		const Material *material_;
		Vec3 normal_; //!< the geometric normal
		const TriangleObject *mesh_;
		size_t self_index_;
		float intersection_bias_factor_;	//!< Intersection Bias factor based on longest edge to reduce
		Vec3 edge_1_, edge_2_;
};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

class TriangleInstance: public Triangle
{
		friend class Scene;
		friend class TriangleObjectInstance;

	public:
		TriangleInstance(): m_base_(nullptr), mesh_(nullptr) { }
		TriangleInstance(Triangle *base, TriangleObjectInstance *m): m_base_(base), mesh_(m) { updateIntersectionCachedValues();}
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const;
		virtual Bound getBound() const;
		virtual bool intersectsBound(ExBound &eb) const;
		virtual bool clippingSupport() const { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const;
		virtual const Material *getMaterial() const { return m_base_->getMaterial(); }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const;
		virtual float surfaceArea() const;
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const;

		virtual Vec3 getNormal() const;
		virtual void recNormal() { /* Empty */ };
		virtual void updateIntersectionCachedValues();

	private:
		const Triangle *m_base_;
		const TriangleObjectInstance *mesh_;
};

/*! inherited triangle, so has virtual functions; connected to meshObject_t;
	otherwise identical to triangle_t
*/

class VTriangle: public Primitive
{
		friend class Scene;
	public:
		VTriangle() {};
		VTriangle(int ia, int ib, int ic, MeshObject *m): pa_(ia), pb_(ib), pc_(ic),
														  na_(-1), nb_(-1), nc_(-1), mesh_(m) { /*recNormal();*/ };
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const;
		virtual Bound getBound() const;
		virtual bool intersectsBound(ExBound &eb) const;
		virtual bool clippingSupport() const { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const;
		virtual const Material *getMaterial() const { return material_; }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const;

		// following are methods which are not part of primitive interface:
		void setMaterial(const Material *m) { material_ = m; }
		void setNormals(int a, int b, int c) { na_ = a, nb_ = b, nc_ = c; }
		Vec3 getNormal() { return Vec3(normal_); }
		float surfaceArea() const;
		void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const;
		void recNormal();

	protected:
		int pa_, pb_, pc_; //!< indices in point array, referenced in mesh.
		int na_, nb_, nc_; //!< indices in normal array, if mesh is smoothed.
		Normal normal_; //!< the geometric normal
		const Material *material_;
		const MeshObject *mesh_;
};

/*! a triangle supporting time based deformation described by a quadratic bezier spline */
class BsTriangle: public Primitive
{
		friend class Scene;
	public:
		BsTriangle() {};
		BsTriangle(int ia, int ib, int ic, MeshObject *m): pa_(ia), pb_(ib), pc_(ic),
														   na_(-1), nb_(-1), nc_(-1), mesh_(m) { };
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const;
		virtual Bound getBound() const;
		//virtual bool intersectsBound(exBound_t &eb) const;
		// return: false:=doesn't overlap bound; true:=valid clip exists
		//virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const;
		virtual const Material *getMaterial() const { return material_; }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const;

		// following are methods which are not part of primitive interface:
		void setMaterial(const Material *m) { material_ = m; }
		void setNormals(int a, int b, int c) { na_ = a, nb_ = b, nc_ = c; }
		//float surfaceArea() const;
		//void sample(float s1, float s2, point3d_t &p, vector3d_t &n) const;

	protected:
		int pa_, pb_, pc_; //!< indices in point array, referenced in mesh.
		int na_, nb_, nc_; //!< indices in normal array, if mesh is smoothed.
		//normal_t normal; //!< the geometric normal
		const Material *material_;
		const MeshObject *mesh_;
};

inline void Triangle::updateIntersectionCachedValues()
{
	Point3 const &a = mesh_->getVertex(pa_);
	Point3 const &b = mesh_->getVertex(pb_);
	Point3 const &c = mesh_->getVertex(pc_);

	edge_1_ = b - a;
	edge_2_ = c - a;

	intersection_bias_factor_ = 0.1f * MIN_RAYDIST * std::max(edge_1_.length(), edge_2_.length());
}


inline void TriangleInstance::updateIntersectionCachedValues()
{
	Point3 const &a = mesh_->getVertex(m_base_->pa_);
	Point3 const &b = mesh_->getVertex(m_base_->pb_);
	Point3 const &c = mesh_->getVertex(m_base_->pc_);

	edge_1_ = b - a;
	edge_2_ = c - a;

	intersection_bias_factor_ = 0.1f * MIN_RAYDIST * std::max(edge_1_.length(), edge_2_.length());
}


inline bool Triangle::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	// Tomas Möller and Ben Trumbore ray intersection scheme
	// Getting the barycentric coordinates of the hit point
	// const point3d_t &a=mesh->points[pa], &b=mesh->points[pb], &c=mesh->points[pc];

	Point3 const &a = mesh_->getVertex(pa_);

	Vec3 pvec = ray.dir_ ^edge_2_;
	float det = edge_1_ * pvec;

	float epsilon = intersection_bias_factor_;

	if(det > -epsilon && det < epsilon) return false;

	float inv_det = 1.f / det;
	Vec3 tvec = ray.from_ - a;
	float u = (tvec * pvec) * inv_det;

	if(u < 0.f || u > 1.f) return false;

	Vec3 qvec = tvec ^edge_1_;
	float v = (ray.dir_ * qvec) * inv_det;

	if((v < 0.f) || ((u + v) > 1.f)) return false;

	*t = edge_2_ * qvec * inv_det;

	if(*t < epsilon) return false;

	data.b_1_ = u;
	data.b_2_ = v;
	data.b_0_ = 1 - u - v;
	data.edge_1_ = &edge_1_;
	data.edge_2_ = &edge_2_;
	return true;
}

inline Bound Triangle::getBound() const
{
	Point3 const &a = mesh_->getVertex(pa_);
	Point3 const &b = mesh_->getVertex(pb_);
	Point3 const &c = mesh_->getVertex(pc_);

	Point3 l, h;
	l.x_ = min3__(a.x_, b.x_, c.x_);
	l.y_ = min3__(a.y_, b.y_, c.y_);
	l.z_ = min3__(a.z_, b.z_, c.z_);
	h.x_ = max3__(a.x_, b.x_, c.x_);
	h.y_ = max3__(a.y_, b.y_, c.y_);
	h.z_ = max3__(a.z_, b.z_, c.z_);
	return Bound(l, h);
}

inline bool Triangle::intersectsBound(ExBound &eb) const
{
	double t_points[3][3];

	Point3 const &a = mesh_->getVertex(pa_);
	Point3 const &b = mesh_->getVertex(pb_);
	Point3 const &c = mesh_->getVertex(pc_);

	for(int j = 0; j < 3; ++j)
	{
		t_points[0][j] = a[j];
		t_points[1][j] = b[j];
		t_points[2][j] = c[j];
	}
	// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
	return triBoxOverlap__(eb.center_, eb.half_size_, (double **) t_points);
}

inline void Triangle::recNormal()
{
	Point3 const &a = mesh_->getVertex(pa_);
	Point3 const &b = mesh_->getVertex(pb_);
	Point3 const &c = mesh_->getVertex(pc_);

	normal_ = ((b - a) ^ (c - a)).normalize();
}

// triangleInstance_t inlined functions

inline bool TriangleInstance::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	// Tomas Möller and Ben Trumbore ray intersection scheme
	// Getting the barycentric coordinates of the hit point
	Point3 const &a = mesh_->getVertex(m_base_->pa_);

	Vec3 pvec = ray.dir_ ^edge_2_;
	float det = edge_1_ * pvec;

	float epsilon = intersection_bias_factor_;

	if(det > -epsilon && det < epsilon) return false;

	float inv_det = 1.f / det;
	Vec3 tvec = ray.from_ - a;
	float u = (tvec * pvec) * inv_det;

	if(u < 0.f || u > 1.f) return false;

	Vec3 qvec = tvec ^edge_1_;
	float v = (ray.dir_ * qvec) * inv_det;

	if((v < 0.f) || ((u + v) > 1.f)) return false;

	*t = edge_2_ * qvec * inv_det;

	if(*t < epsilon) return false;

	data.b_1_ = u;
	data.b_2_ = v;
	data.b_0_ = 1 - u - v;
	data.edge_1_ = &edge_1_;
	data.edge_2_ = &edge_2_;
	return true;
}

inline Bound TriangleInstance::getBound() const
{
	Point3 const &a = mesh_->getVertex(m_base_->pa_);
	Point3 const &b = mesh_->getVertex(m_base_->pb_);
	Point3 const &c = mesh_->getVertex(m_base_->pc_);

	Point3 l, h;
	l.x_ = min3__(a.x_, b.x_, c.x_);
	l.y_ = min3__(a.y_, b.y_, c.y_);
	l.z_ = min3__(a.z_, b.z_, c.z_);
	h.x_ = max3__(a.x_, b.x_, c.x_);
	h.y_ = max3__(a.y_, b.y_, c.y_);
	h.z_ = max3__(a.z_, b.z_, c.z_);
	return Bound(l, h);
}

inline bool TriangleInstance::intersectsBound(ExBound &eb) const
{
	double t_points[3][3];

	Point3 const &a = mesh_->getVertex(m_base_->pa_);
	Point3 const &b = mesh_->getVertex(m_base_->pb_);
	Point3 const &c = mesh_->getVertex(m_base_->pc_);

	for(int j = 0; j < 3; ++j)
	{
		t_points[0][j] = a[j];
		t_points[1][j] = b[j];
		t_points[2][j] = c[j];
	}
	// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
	return triBoxOverlap__(eb.center_, eb.half_size_, (double **) t_points);
}

inline Vec3 TriangleInstance::getNormal() const
{
	return Vec3(mesh_->obj_to_world_ * m_base_->normal_).normalize();
}


END_YAFARAY

#endif // YAFARAY_TRIANGLE_H
