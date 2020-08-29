#pragma once
/****************************************************************************
 *
 * 			surface.h: Surface sampling representation and api
 *      This is part of the yafray package
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

#include <yafray_constants.h>
#include "vector3d.h"
#include "color.h"


BEGIN_YAFRAY
class Material;
class Light;
class Object3D;
class DiffRay;
class Vec3;

class IntersectData
{
	public:
		IntersectData()
		{
			// Empty
		}
		float b_0_ = 0.f;
		float b_1_ = 0.f;
		float b_2_ = 0.f;
		float t_ = 0.f;
		const Vec3 *edge_1_ = nullptr;
		const Vec3 *edge_2_ = nullptr;
};

/*! This holds a sampled surface point's data
	When a ray intersects an object, a surfacePoint_t is computed.
	It contains data about normal, position, assigned material and other
	things.
 */
class YAFRAYCORE_EXPORT SurfacePoint
{
	public:
		float getDistToNearestEdge() const;

		//int object; //!< the object owner of the point.
		const Material *material_; //!< the surface material
		const Light *light_; //!< light source if surface point is on a light
		const Object3D *object_; //!< object the prim belongs to
		//	point2d_t screenpos; // only used with 'win' texture coord. mode
		void *origin_;
		IntersectData data_;

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
	if(data_.edge_1_ && data_.edge_2_)
	{
		float edge_1_len = data_.edge_1_->length();
		float edge_2_len = data_.edge_2_->length();
		float edge_12_len = (*(data_.edge_1_) + * (data_.edge_2_)).length() * 0.5f;

		float edge_1_dist = data_.b_1_ * edge_1_len;
		float edge_2_dist = data_.b_2_ * edge_2_len;
		float edge_12_dist = data_.b_0_ * edge_12_len;

		float edge_min_dist = std::min(edge_12_dist, std::min(edge_1_dist, edge_2_dist));

		return edge_min_dist;
	}
	else return std::numeric_limits<float>::infinity();
}


YAFRAYCORE_EXPORT SurfacePoint blendSurfacePoints__(SurfacePoint const &sp_0, SurfacePoint const &sp_1, float const alpha);

/*! computes and stores the additional data for surface intersections for
	differential rays */
class YAFRAYCORE_EXPORT SpDifferentials
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

END_YAFRAY

#endif
