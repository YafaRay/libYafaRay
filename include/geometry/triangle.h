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
#include "geometry/object_geom_mesh.h"
#include "geometry/primitive.h"
#include "bound.h"
#include "surface.h"

BEGIN_YAFARAY

// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
int triBoxOverlap__(const double *boxcenter, const double *boxhalfsize, double **triverts);


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
		friend class YafaRayScene;
		friend class TriangleObject;
		friend class TriangleInstance;

	public:
		Triangle() = default;
		Triangle(int ia, int ib, int ic, TriangleObject *m): pa_(ia), pb_(ib), pc_(ic), mesh_(m) {  updateIntersectionCachedValues(); }
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const;
		virtual Bound getBound() const;
		virtual bool intersectsBound(const ExBound &eb) const;
		virtual bool clippingSupport() const { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(const double bound[2][3], int axis, Bound &clipped, const void *d_old, void *d_new) const;
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
		bool operator == (Triangle const &a) const { return self_index_ == a.self_index_; }
		friend std::ostream &operator << (std::ostream &out, const Triangle &t)
		{
			out << "[ idx = " << t.self_index_ << " (" << t.pa_ << "," << t.pb_ << "," << t.pc_ << ")]";
			return out;
		}
		virtual const TriangleObject *getMesh() const { return mesh_; }

	private:
		virtual void updateIntersectionCachedValues();

		int pa_ = -1, pb_ = -1, pc_ = -1; //!< indices in point array, referenced in mesh.
		int na_ = -1, nb_ = -1, nc_ = -1; //!< indices in normal array, if mesh is smoothed.
		const Material *material_ = nullptr;
		Vec3 normal_; //!< the geometric normal
		const TriangleObject *mesh_ = nullptr;
		size_t self_index_ = 0;
		float intersection_bias_factor_ = 0.f;	//!< Intersection Bias factor based on longest edge to reduce
		Vec3 edge_1_{0.f}, edge_2_{0.f};
};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

class TriangleInstance: public Triangle
{
		friend class Scene;
		friend class YafaRayScene;
		friend class TriangleObjectInstance;

	public:
		TriangleInstance() = default;
		TriangleInstance(const Triangle *base, TriangleObjectInstance *m): m_base_(base), mesh_(m) { updateIntersectionCachedValues();}
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const override;
		virtual Bound getBound() const override;
		virtual bool intersectsBound(const ExBound &eb) const override;
		virtual bool clippingSupport() const override { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(const double bound[2][3], int axis, Bound &clipped, const void *d_old, void *d_new) const override;
		virtual const Material *getMaterial() const override { return m_base_->getMaterial(); }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const override;
		virtual float surfaceArea() const override;
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const override;
		virtual Vec3 getNormal() const override;
		virtual void recNormal() override { /* Empty */ };

	private:
		virtual void updateIntersectionCachedValues() override;

		const Triangle *m_base_ = nullptr;
		const TriangleObjectInstance *mesh_ = nullptr;
};

/*! inherited triangle, so has virtual functions; connected to meshObject_t;
	otherwise identical to triangle_t
*/

class VTriangle: public Primitive
{
		friend class Scene;
		friend class YafaRayScene;
	public:
		VTriangle() = default;
		VTriangle(int ia, int ib, int ic, MeshObject *m): pa_(ia), pb_(ib), pc_(ic), mesh_(m) { /*recNormal();*/ };
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const override;
		virtual Bound getBound() const override;
		virtual bool intersectsBound(ExBound &eb) const override;
		virtual bool clippingSupport() const override { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		virtual bool clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const override;
		virtual const Material *getMaterial() const override { return material_; }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const override;

		// following are methods which are not part of primitive interface:
		void setMaterial(const Material *m) { material_ = m; }
		void setNormals(int a, int b, int c) { na_ = a, nb_ = b, nc_ = c; }
		Vec3 getNormal() { return Vec3(normal_); }
		float surfaceArea() const;
		void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const;
		void recNormal();

	protected:
		int pa_ = -1, pb_ = -1, pc_ = -1; //!< indices in point array, referenced in mesh.
		int na_ = -1, nb_ = -1, nc_ = -1; //!< indices in normal array, if mesh is smoothed.
		Vec3 normal_; //!< the geometric normal
		const Material *material_ = nullptr;
		const MeshObject *mesh_ = nullptr;
};

/*! a triangle supporting time based deformation described by a quadratic bezier spline */
class BsTriangle: public Primitive
{
		friend class Scene;
		friend class YafaRayScene;
	public:
		BsTriangle() = default;
		BsTriangle(int ia, int ib, int ic, MeshObject *m): pa_(ia), pb_(ib), pc_(ic), mesh_(m) { };
		virtual bool intersect(const Ray &ray, float *t, IntersectData &data) const override;
		virtual Bound getBound() const override;
		//virtual bool intersectsBound(exBound_t &eb) const;
		// return: false:=doesn't overlap bound; true:=valid clip exists
		//virtual bool clipToBound(double bound[2][3], int axis, bound_t &clipped, void *d_old, void *d_new) const;
		virtual const Material *getMaterial() const override { return material_; }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const override;

		// following are methods which are not part of primitive interface:
		void setMaterial(const Material *m) { material_ = m; }
		void setNormals(int a, int b, int c) { na_ = a, nb_ = b, nc_ = c; }
		//float surfaceArea() const;
		//void sample(float s1, float s2, point3d_t &p, vector3d_t &n) const;

	protected:
		int pa_ = -1, pb_ = -1, pc_ = -1; //!< indices in point array, referenced in mesh.
		int na_ = -1, nb_ = -1, nc_ = -1; //!< indices in normal array, if mesh is smoothed.
		//normal_t normal; //!< the geometric normal
		const Material *material_ = nullptr;
		const MeshObject *mesh_ = nullptr;
};

inline void Triangle::updateIntersectionCachedValues()
{
	const Point3 &a = mesh_->getVertex(pa_);
	const Point3 &b = mesh_->getVertex(pb_);
	const Point3 &c = mesh_->getVertex(pc_);
	edge_1_ = b - a;
	edge_2_ = c - a;
	intersection_bias_factor_ = 0.1f * min_raydist__ * std::max(edge_1_.length(), edge_2_.length());
}


inline void TriangleInstance::updateIntersectionCachedValues()
{
	const Point3 &a = mesh_->getVertex(m_base_->pa_);
	const Point3 &b = mesh_->getVertex(m_base_->pb_);
	const Point3 &c = mesh_->getVertex(m_base_->pc_);
	edge_1_ = b - a;
	edge_2_ = c - a;
	intersection_bias_factor_ = 0.1f * min_raydist__ * std::max(edge_1_.length(), edge_2_.length());
}

inline bool Triangle::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	// Tomas Möller and Ben Trumbore ray intersection scheme
	// Getting the barycentric coordinates of the hit point
	// const point3d_t &a=mesh->points[pa], &b=mesh->points[pb], &c=mesh->points[pc];

	const Point3 &a = mesh_->getVertex(pa_);
	const Vec3 pvec = ray.dir_ ^ edge_2_;
	const float det = edge_1_ * pvec;
	const float epsilon = intersection_bias_factor_;
	if(det > -epsilon && det < epsilon) return false;
	const float inv_det = 1.f / det;
	const Vec3 tvec = ray.from_ - a;
	const float u = (tvec * pvec) * inv_det;
	if(u < 0.f || u > 1.f) return false;
	const Vec3 qvec = tvec ^edge_1_;
	const float v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.f) || ((u + v) > 1.f)) return false;
	*t = edge_2_ * qvec * inv_det;
	if(*t < epsilon) return false;
	//UV <-> Barycentric UVW relationship is not obvious, interesting explanation in: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
	data.barycentric_u_ = 1.f - u - v;
	data.barycentric_v_ = u;
	data.barycentric_w_ = v;
	data.time_ = ray.time_;
	return true;
}

inline Bound Triangle::getBound() const
{
	const Point3 &a = mesh_->getVertex(pa_);
	const Point3 &b = mesh_->getVertex(pb_);
	const Point3 &c = mesh_->getVertex(pc_);
	const Point3 l { math::min(a.x_, b.x_, c.x_), math::min(a.y_, b.y_, c.y_), math::min(a.z_, b.z_, c.z_) };
	const Point3 h { math::max(a.x_, b.x_, c.x_), math::max(a.y_, b.y_, c.y_), math::max(a.z_, b.z_, c.z_) };
	return Bound(l, h);
}

inline bool Triangle::intersectsBound(const ExBound &eb) const
{
	const Point3 &a = mesh_->getVertex(pa_);
	const Point3 &b = mesh_->getVertex(pb_);
	const Point3 &c = mesh_->getVertex(pc_);
	double t_points[3][3];
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
	const Point3 &a = mesh_->getVertex(pa_);
	const Point3 &b = mesh_->getVertex(pb_);
	const Point3 &c = mesh_->getVertex(pc_);
	normal_ = ((b - a) ^ (c - a)).normalize();
}

// triangleInstance_t inlined functions

inline bool TriangleInstance::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	// Tomas Möller and Ben Trumbore ray intersection scheme
	// Getting the barycentric coordinates of the hit point
	const Point3 &a = mesh_->getVertex(m_base_->pa_);
	const Vec3 pvec = ray.dir_ ^edge_2_;
	const float det = edge_1_ * pvec;
	const float epsilon = intersection_bias_factor_;
	if(det > -epsilon && det < epsilon) return false;
	const float inv_det = 1.f / det;
	const Vec3 tvec = ray.from_ - a;
	const float u = (tvec * pvec) * inv_det;
	if(u < 0.f || u > 1.f) return false;
	const Vec3 qvec = tvec ^edge_1_;
	const float v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.f) || ((u + v) > 1.f)) return false;
	*t = edge_2_ * qvec * inv_det;
	if(*t < epsilon) return false;
	//UV <-> Barycentric UVW relationship is not obvious, interesting explanation in: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
	data.barycentric_u_ = 1.f - u - v;
	data.barycentric_v_ = u;
	data.barycentric_w_ = v;
	data.time_ = ray.time_;
	return true;
}

inline Bound TriangleInstance::getBound() const
{
	const Point3 &a = mesh_->getVertex(m_base_->pa_);
	const Point3 &b = mesh_->getVertex(m_base_->pb_);
	const Point3 &c = mesh_->getVertex(m_base_->pc_);
	const Point3 l { math::min(a.x_, b.x_, c.x_), math::min(a.y_, b.y_, c.y_), math::min(a.z_, b.z_, c.z_) };
	const Point3 h { math::max(a.x_, b.x_, c.x_), math::max(a.y_, b.y_, c.y_), math::max(a.z_, b.z_, c.z_) };
	return Bound(l, h);
}

inline bool TriangleInstance::intersectsBound(const ExBound &eb) const
{
	const Point3 &a = mesh_->getVertex(m_base_->pa_);
	const Point3 &b = mesh_->getVertex(m_base_->pb_);
	const Point3 &c = mesh_->getVertex(m_base_->pc_);
	double t_points[3][3];
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
