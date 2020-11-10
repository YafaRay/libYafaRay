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
#include "geometry/vector.h"
#include <array>

BEGIN_YAFARAY

class Ray;
class TriangleObject;
class Bound;
class ExBound;
class IntersectData;
class Material;
class SurfacePoint;
class Uv;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

class Triangle
{
	public:
		Triangle() = default;
		Triangle(int ia, int ib, int ic, TriangleObject *m): point_id_({ia, ib, ic}), triangle_object_(m) { updateIntersectCachedValues(); }
		bool intersect(const Ray &ray, float *t, IntersectData &intersect_data) const;
		Bound getBound() const;
		bool intersectsBound(const ExBound &ex_bound) const;
		virtual bool clippingSupport() const { return true; }
		// return: false:=doesn't overlap bound; true:=valid clip exists
		bool clipToBound(const double bound[2][3], int axis, Bound &clipped, const void *d_old, void *d_new) const;
		virtual const Material *getMaterial() const { return material_; }
		virtual void getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const;
		float surfaceArea() const { return surfaceArea(getVertices()); }
		static float surfaceArea(const std::array<Point3, 3> &vertices);
		virtual void sample(float s_1, float s_2, Point3 &p, Vec3 &n) const;
		static void sample(float s_1, float s_2, Point3 &p, const std::array<Point3, 3> &vertices);
		virtual Vec3 getNormal() const { return Vec3(geometric_normal_); }
		void setVertexIndices(int a, int b, int c) { point_id_ = {a, b, c}; updateIntersectCachedValues(); }
		void setMaterial(const Material *m) { material_ = m; }
		void setNormalsIndices(std::array<int, 3> normals_indices) { normal_id_ = normals_indices; }
		virtual void recNormal();
		virtual size_t getSelfIndex() const { return self_index_; }
		virtual void setSelfIndex(size_t index) { self_index_ = index; }
		bool operator == (Triangle const &a) const { return self_index_ == a.self_index_; }
		friend std::ostream &operator << (std::ostream &out, const Triangle &t);
		virtual const TriangleObject *getMesh() const { return triangle_object_; }
		std::array<Point3, 3> getVertices() const;
		std::array<Point3, 3> getOrcoVertices() const;
		std::array<Vec3, 3> getVerticesNormals(const Vec3 &surface_normal) const;
		std::array<Uv, 3> getVerticesUvs() const;
		std::array<int, 3> getVerticesIndices() const { return point_id_; }
		std::array<int, 3> getNormalsIndices() const { return normal_id_; }
		virtual Point3 getVertex(size_t index) const; //!< Get triangle vertex (index is the vertex number in the triangle: 0, 1 or 2
		virtual Point3 getOrcoVertex(size_t index) const; //!< Get triangle original coordinates (orco) vertex in instance objects (index is the vertex number in the triangle: 0, 1 or 2
		virtual Point3 getVertexNormal(size_t index, const Vec3 &surface_normal) const; //!< Get triangle vertex normal (index is the vertex number in the triangle: 0, 1 or 2
		virtual Uv getVertexUv(size_t index) const; //!< Get triangle vertex Uv (index is the vertex number in the triangle: 0, 1 or 2
		int getPointId(size_t index) const { return point_id_[index]; }  //!< Get triangle Point Id (index is the vertex number in the triangle: 0, 1 or 2
		int getNormalId(size_t index) const { return normal_id_[index]; }  //!< Get triangle Normal Id (index is the vertex number in the triangle: 0, 1 or 2

		static int triBoxClip(const double b_min[3], const double b_max[3], const double triverts[3][3], Bound &box, void *n_dat);
		static int triPlaneClip(double pos, int axis, bool lower, Bound &box, const void *o_dat, void *n_dat);
		static bool intersect(const Ray &ray, float *t, IntersectData &intersect_data, const Point3 &p_0, const Vec3 &vec_0_1, const Vec3 &vec_0_2, float intersection_bias_factor);
		static Bound getBound(const std::array<Point3, 3> &triangle_verts);
		static bool intersectsBound(const ExBound &ex_bound, const std::array<Point3, 3> &triangle_verts);
		static Vec3 calculateNormal(const std::array<Point3, 3> &triangle_verts);
		virtual void calculateShadingSpace(SurfacePoint &sp) const;

	protected:
		void updateIntersectCachedValues();

	private:
		std::array<int, 3> point_id_ {-1, -1, -1 }; //!< indices in point array, referenced in mesh.
		std::array<int, 3> normal_id_ {-1, -1, -1 }; //!< indices in normal array, if mesh is smoothed.
		const Material *material_ = nullptr;
		Vec3 geometric_normal_;
		const TriangleObject *triangle_object_ = nullptr;
		size_t self_index_ = 0;
		float intersect_bias_factor_ = 0.f; //!< Intersection Bias factor based on longest edge to reduce
		Vec3 vec_0_1_{0.f}, vec_0_2_{0.f};
};

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

//triBoxOverlap comes from tribox3_d.cc (AABB-triangle overlap test code by Tomas Akenine-Möller)
extern int triBoxOverlap__(const double *boxcenter, const double *boxhalfsize, double **triverts);

END_YAFARAY

#endif // YAFARAY_TRIANGLE_H
