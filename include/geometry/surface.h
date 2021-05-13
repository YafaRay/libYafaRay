#pragma once
/****************************************************************************
 *
 *      surface.h: Surface sampling representation and api
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002 Alejandro Conty Estévez
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
 *
 */
#ifndef YAFARAY_SURFACE_H
#define YAFARAY_SURFACE_H

#include "yafaray_conf.h"
#include "geometry/vector.h"
#include "geometry/intersect_data.h"
#include "color/color.h"

BEGIN_YAFARAY
class Material;
class Light;
class Object;
class DiffRay;
class Vec3;
class Primitive;
struct IntersectData;

/*! This holds a sampled surface point's data
	When a ray intersects an object, a surfacePoint_t is computed.
	It contains data about normal, position, assigned material and other
	things.
 */
class SurfacePoint
{
	public:
		static Vec3 normalFaceForward(const Vec3 &normal_geometry, const Vec3 &normal, const Vec3 &incoming_vector);
		static SurfacePoint blendSurfacePoints(SurfacePoint const &sp_0, SurfacePoint const &sp_1, float alpha);
		float getDistToNearestEdge() const;

		//int object; //!< the object owner of the point.
		const Material *material_; //!< the surface material
		const Light *light_; //!< light source if surface point is on a light
		const Object *object_; //!< object the prim belongs to
		//	point2d_t screenpos; // only used with 'win' texture coord. mode
		const Primitive *hit_primitive_;
		IntersectData intersect_data_;

		// Geometry related
		Vec3 n_; //!< the shading normal.
		Vec3 ng_; //!< the geometric normal.
		Vec3 orco_ng_; //!< the untransformed geometric normal.
		Point3 p_; //!< the (world) position.
		Point3 orco_p_;
		//	Rgb vertex_col;
		bool has_uv_;
		bool has_orco_;
		bool available_;
		int prim_num_;

		float u_; //!< the u texture coord.
		float v_; //!< the v texture coord.
		Vec3 nu_; //!< second vector building orthogonal shading space with N
		Vec3 nv_; //!< third vector building orthogonal shading space with N
		Vec3 dp_du_; //!< u-axis in world space (normalized)
		Vec3 dp_dv_; //!< v-axis in world space (normalized)
		Vec3 ds_du_; //!< u-axis in shading space (NU, NV, N)
		Vec3 ds_dv_; //!< v-axis in shading space (NU, NV, N)
		Vec3 dp_du_abs_; //!< u-axis in world space (before normalization)
		Vec3 dp_dv_abs_; //!< v-axis in world space (before normalization)
		//float dudNU;
		//float dudNV;
		//float dvdNU;
		//float dvdNV;

		// Differential ray for mipmaps calculations
		const DiffRay *ray_ = nullptr;
};

inline float SurfacePoint::getDistToNearestEdge() const
{
	const float u_dist_rel = 0.5f - std::abs(intersect_data_.barycentric_u_ - 0.5f);
	const float u_dist_abs = u_dist_rel * dp_du_abs_.length();
	const float v_dist_rel = 0.5f - std::abs(intersect_data_.barycentric_v_ - 0.5f);
	const float v_dist_abs = v_dist_rel * dp_dv_abs_.length();
	const float w_dist_rel = 0.5f - std::abs(intersect_data_.barycentric_w_ - 0.5f);
	const float w_dist_abs = w_dist_rel * (dp_dv_abs_ - dp_du_abs_).length();
	return math::min(u_dist_abs, v_dist_abs, w_dist_abs);
}

inline Vec3 SurfacePoint::normalFaceForward(const Vec3 &normal_geometry, const Vec3 &normal, const Vec3 &incoming_vector)
{
	return (normal_geometry * incoming_vector) < 0 ? -normal : normal;
}

/*! computes and stores the additional data for surface intersections for
	differential rays */
class SpDifferentials
{
	public:
		SpDifferentials(const SurfacePoint &spoint, const DiffRay &ray);
		//! compute differentials for a scattered ray
		void reflectedRay(const DiffRay &in, DiffRay &out) const;
		//! compute differentials for a refracted ray
		void refractedRay(const DiffRay &in, DiffRay &out, float ior) const;
		float projectedPixelArea();
		void getUVdifferentials(float &du_dx, float &dv_dx, float &du_dy, float &dv_dy) const;
		Vec3 dp_dx_;
		Vec3 dp_dy_;
		const SurfacePoint &sp_;
	protected:
		void dUdvFromDpdPdUdPdV(float &du, float &dv, const Point3 &dp, const Vec3 &dp_du, const Vec3 &dp_dv) const;
};

END_YAFARAY

#endif
