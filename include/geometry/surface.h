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

#include "common/yafaray_common.h"
#include "geometry/vector.h"
#include "geometry/intersect_data.h"
#include "color/color.h"
#include "material/material.h"

BEGIN_YAFARAY
class Light;
class Object;
class Ray;
class Vec3;
class Primitive;
struct RayDifferentials;
struct IntersectData;

struct SurfaceDifferentials final
{
	SurfaceDifferentials() = default;
	SurfaceDifferentials(const SurfaceDifferentials &surface_differentials) = default;
	SurfaceDifferentials(SurfaceDifferentials &&surface_differentials) = default;
	SurfaceDifferentials(const Vec3 &dp_dx, const Vec3 &dp_dy) : dp_dx_(dp_dx), dp_dy_(dp_dy) { }
	Vec3 dp_dx_;
	Vec3 dp_dy_;
};

/*! This holds a sampled surface point's data
	When a ray intersects an object, a surfacePoint_t is computed.
	It contains data about normal, position, assigned material and other
	things.
 */
class SurfacePoint final
{
	public:
		SurfacePoint() = default;
		SurfacePoint(const SurfacePoint &sp);
		SurfacePoint& operator=(const SurfacePoint &sp);
		SurfacePoint(SurfacePoint &&surface_point) = default;
		SurfacePoint& operator=(SurfacePoint&& surface_point) = default;
		void calculateShadingSpace();
		static Vec3 normalFaceForward(const Vec3 &normal_geometry, const Vec3 &normal, const Vec3 &incoming_vector);
		static SurfacePoint blendSurfacePoints(SurfacePoint const &sp_1, SurfacePoint const &sp_2, float alpha);
		float getDistToNearestEdge() const;
		//! compute differentials for a scattered ray
		std::unique_ptr<RayDifferentials> reflectedRay(const RayDifferentials *in_differentials, const Vec3 &in_dir, const Vec3 &out_dir) const;
		//! compute differentials for a refracted ray
		std::unique_ptr<RayDifferentials> refractedRay(const RayDifferentials *in_differentials, const Vec3 &in_dir, const Vec3 &out_dir, float ior) const;
		float projectedPixelArea() const;
		void getUVdifferentials(float &du_dx, float &dv_dx, float &du_dy, float &dv_dy) const;
		void setRayDifferentials(const RayDifferentials *ray_differentials);

		const MaterialData * initBsdf(const Camera *camera);
		Rgb eval(const Vec3 &wo, const Vec3 &wl, const BsdfFlags &types, bool force_eval = false) const;
		Rgb sample(const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const;
		Rgb sample(const Vec3 &wo, Vec3 *dir, Rgb &tcol, Sample &s, float *w, bool chromatic, float wavelength) const;
		float pdf(const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const;
		Rgb getTransparency(const Vec3 &wo, const Camera *camera) const;
		Specular getSpecular(int ray_level, const Vec3 &wo, bool chromatic, float wavelength) const;
		Rgb getReflectivity(FastRandom &fast_random, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const;
		Rgb emit(const Vec3 &wo) const;
		float getAlpha(const Vec3 &wo, const Camera *camera) const;
		bool scatterPhoton(const Vec3 &wi, Vec3 &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const;

		//int object; //!< the object owner of the point.
		const Material *material_; //!< the surface material
		std::unique_ptr<const MaterialData> mat_data_;
		const Light *light_; //!< light source if surface point is on a light
		const Primitive *primitive_; //!< primitive the surface belongs to
		//	point2d_t screenpos; // only used with 'win' texture coord. mode
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
		// Surface Differentials for mipmaps calculations
		std::unique_ptr<const SurfaceDifferentials> differentials_;

	private:
		static void dUdvFromDpdPdUdPdV(float &du, float &dv, const Point3 &dp, const Vec3 &dp_du, const Vec3 &dp_dv);
};

inline SurfacePoint::SurfacePoint(const SurfacePoint &sp)
	:
	material_{sp.material_},
	light_{sp.light_},
	primitive_{sp.primitive_},
	intersect_data_{sp.intersect_data_},
	n_{sp.n_},
	ng_{sp.ng_},
	orco_ng_{sp.orco_ng_},
	p_{sp.p_},
	orco_p_{sp.orco_p_},
	has_uv_{sp.has_uv_},
	has_orco_{sp.has_orco_},
	u_{sp.u_},
	v_{sp.v_},
	nu_{sp.nu_},
	nv_{sp.nv_},
	dp_du_{sp.dp_du_},
	dp_dv_{sp.dp_dv_},
	ds_du_{sp.ds_du_},
	ds_dv_{sp.ds_dv_},
	dp_du_abs_{sp.dp_du_abs_},
	dp_dv_abs_{sp.dp_dv_abs_}
{
	if(sp.mat_data_) mat_data_ = sp.mat_data_->clone();
	if(sp.differentials_) differentials_ = std::make_unique<const SurfaceDifferentials>(*sp.differentials_);
}

inline SurfacePoint &SurfacePoint::operator=(const SurfacePoint &sp)
{
	if(this == &sp) return *this;
	material_ = sp.material_;
	if(sp.mat_data_) mat_data_ = sp.mat_data_->clone();
	light_ = sp.light_;
	primitive_ = sp.primitive_;
	intersect_data_ = sp.intersect_data_;
	n_ = sp.n_;
	ng_ = sp.ng_;
	orco_ng_ = sp.orco_ng_;
	p_ = sp.p_;
	orco_p_ = sp.orco_p_;
	has_uv_ = sp.has_uv_;
	has_orco_ = sp.has_orco_;
	u_ = sp.u_;
	v_ = sp.v_;
	nu_ = sp.nu_;
	nv_ = sp.nv_;
	dp_du_ = sp.dp_du_;
	dp_dv_ = sp.dp_dv_;
	ds_du_ = sp.ds_du_;
	ds_dv_ = sp.ds_dv_;
	dp_du_abs_ = sp.dp_du_abs_;
	dp_dv_abs_ = sp.dp_dv_abs_;
	if(sp.differentials_) differentials_ = std::make_unique<const SurfaceDifferentials>(*sp.differentials_);
	return *this;
}

inline void SurfacePoint::calculateShadingSpace()
{
	// transform dPdU and dPdV in shading space
	ds_du_.x() = nu_ * dp_du_;
	ds_du_.y() = nv_ * dp_du_;
	ds_du_.z() = n_ * dp_du_;
	ds_dv_.x() = nu_ * dp_dv_;
	ds_dv_.y() = nv_ * dp_dv_;
	ds_dv_.z() = n_ * dp_dv_;
}

inline Vec3 SurfacePoint::normalFaceForward(const Vec3 &normal_geometry, const Vec3 &normal, const Vec3 &incoming_vector)
{
	return (normal_geometry * incoming_vector) < 0 ? -normal : normal;
}

inline const MaterialData * SurfacePoint::initBsdf(const Camera *camera)
{
	return material_->initBsdf(*this, camera);
}

inline Rgb SurfacePoint::eval(const Vec3 &wo, const Vec3 &wl, const BsdfFlags &types, bool force_eval) const
{
	return material_->eval(mat_data_.get(), *this, wo, wl, types, force_eval);
}

inline Rgb SurfacePoint::sample(const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	return material_->sample(mat_data_.get(), *this, wo, wi, s, w, chromatic, wavelength, camera);
}

inline Rgb SurfacePoint::sample(const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const
{
	return material_->sample(mat_data_.get(), *this, wo, dir, tcol, s, w, chromatic, wavelength);
}

inline float SurfacePoint::pdf(const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	return material_->pdf(mat_data_.get(), *this, wo, wi, bsdfs);
}

inline Rgb SurfacePoint::getTransparency(const Vec3 &wo, const Camera *camera) const
{
	return material_->getTransparency(mat_data_.get(), *this, wo, camera);
}

inline Specular SurfacePoint::getSpecular(int ray_level, const Vec3 &wo, bool chromatic, float wavelength) const
{
	return material_->getSpecular(ray_level, mat_data_.get(), *this, wo, chromatic, wavelength);
}

inline Rgb SurfacePoint::getReflectivity(FastRandom &fast_random, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const
{
	return material_->getReflectivity(fast_random, mat_data_.get(), *this, flags, chromatic, wavelength, camera);
}

inline Rgb SurfacePoint::emit(const Vec3 &wo) const
{
	return material_->emit(mat_data_.get(), *this, wo);
}

inline float SurfacePoint::getAlpha(const Vec3 &wo, const Camera *camera) const
{
	return material_->getAlpha(mat_data_.get(), *this, wo, camera);
}

inline bool SurfacePoint::scatterPhoton(const Vec3 &wi, Vec3 &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const
{
	return material_->scatterPhoton(mat_data_.get(), *this, wi, wo, s, chromatic, wavelength, camera);
}

END_YAFARAY

#endif
