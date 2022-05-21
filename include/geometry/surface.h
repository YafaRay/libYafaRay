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
#include "geometry/object/object.h"
#include "geometry/primitive/primitive.h"
#include "color/color.h"
#include "material/material.h"
#include "math/interpolation.h"

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
		SurfacePoint(const Primitive *primitive) : primitive_(primitive) { }
		SurfacePoint(const SurfacePoint &sp);
		SurfacePoint(const SurfacePoint &sp_1, const SurfacePoint &sp_2, float alpha);
		SurfacePoint& operator=(const SurfacePoint &sp);
		SurfacePoint(SurfacePoint &&surface_point) = default;
		SurfacePoint& operator=(SurfacePoint&& surface_point) = default;
		static Vec3 normalFaceForward(const Vec3 &normal_geometry, const Vec3 &normal, const Vec3 &incoming_vector);
		float getDistToNearestEdge() const;
		//! compute differentials for a scattered ray
		std::unique_ptr<RayDifferentials> reflectedRay(const RayDifferentials *in_differentials, const Vec3 &in_dir, const Vec3 &out_dir) const;
		//! compute differentials for a refracted ray
		std::unique_ptr<RayDifferentials> refractedRay(const RayDifferentials *in_differentials, const Vec3 &in_dir, const Vec3 &out_dir, float ior) const;
		float projectedPixelArea() const;
		void getUVdifferentials(float &du_dx, float &dv_dx, float &du_dy, float &dv_dy) const;
		std::unique_ptr<SurfaceDifferentials> calcSurfaceDifferentials(const RayDifferentials *ray_differentials) const;

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
		const Object *getObject() const { if(primitive_) return primitive_->getObject(); else return nullptr; }
		const Material *getMaterial() const { if(primitive_) return primitive_->getMaterial(); else return nullptr; }
		const Light *getLight() const { if(primitive_) return primitive_->getObject()->getLight(); else return nullptr; }

		IntersectData intersect_data_;
		alignas(8) std::unique_ptr<const MaterialData> mat_data_;
		std::unique_ptr<const SurfaceDifferentials> differentials_; //!< Surface Differentials for mipmaps calculations

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

	private:
		static void dUdvFromDpdPdUdPdV(float &du, float &dv, const Point3 &dp, const Vec3 &dp_du, const Vec3 &dp_dv);
		const Primitive *primitive_ = nullptr; //!< primitive the surface belongs to
};

inline SurfacePoint::SurfacePoint(const SurfacePoint &sp)
	:
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

inline SurfacePoint::SurfacePoint(const SurfacePoint &sp_1, const SurfacePoint &sp_2, float alpha)
	:
	  primitive_{alpha < 0.5f ? sp_1.primitive_ : sp_2.primitive_},
	  intersect_data_{alpha < 0.5f ? sp_1.intersect_data_ : sp_2.intersect_data_},
	  n_{math::lerp(sp_1.n_, sp_2.n_, alpha)},
	  ng_{alpha < 0.5f ? sp_1.ng_ : sp_2.ng_},
	  orco_ng_{alpha < 0.5f ? sp_1.orco_ng_ : sp_2.orco_ng_},
	  p_{alpha < 0.5f ? sp_1.p_ : sp_2.p_},
	  orco_p_{alpha < 0.5f ? sp_1.orco_p_ : sp_2.orco_p_},
	  has_uv_{alpha < 0.5f ? sp_1.has_uv_ : sp_2.has_uv_},
	  has_orco_{alpha < 0.5f ? sp_1.has_orco_ : sp_2.has_orco_},
	  u_{alpha < 0.5f ? sp_1.u_ : sp_2.u_},
	  v_{alpha < 0.5f ? sp_1.v_ : sp_2.v_},
	  nu_{math::lerp(sp_1.nu_, sp_2.nu_, alpha)},
	  nv_{math::lerp(sp_1.nv_, sp_2.nv_, alpha)},
	  dp_du_{math::lerp(sp_1.dp_du_, sp_2.dp_du_, alpha)},
	  dp_dv_{math::lerp(sp_1.dp_dv_, sp_2.dp_dv_, alpha)},
	  ds_du_{math::lerp(sp_1.ds_du_, sp_2.ds_du_, alpha)},
	  ds_dv_{math::lerp(sp_1.ds_dv_, sp_2.ds_dv_, alpha)},
	  dp_du_abs_{alpha < 0.5f ? sp_1.dp_du_abs_ : sp_2.dp_du_abs_},
	  dp_dv_abs_{alpha < 0.5f ? sp_1.dp_dv_abs_ : sp_2.dp_dv_abs_}
{
	if(alpha < 0.5f)
	{
		if(sp_1.mat_data_) mat_data_ = sp_1.mat_data_->clone();
	}
	else
	{
		if(sp_2.mat_data_) mat_data_ = sp_2.mat_data_->clone();
	}
	if(sp_1.differentials_ && sp_2.differentials_)
	{
		differentials_ = std::make_unique<SurfaceDifferentials>(
				math::lerp(sp_1.differentials_->dp_dx_, sp_2.differentials_->dp_dx_, alpha),
				math::lerp(sp_1.differentials_->dp_dy_, sp_2.differentials_->dp_dy_, alpha)
		); //FIXME: should this std::max or std::min instead of lerp?
	}
	else if(sp_1.differentials_)
	{
		differentials_ = std::make_unique<SurfaceDifferentials>(*sp_1.differentials_);
	}
	else if(sp_2.differentials_)
	{
		differentials_ = std::make_unique<SurfaceDifferentials>(*sp_2.differentials_);
	}
}

inline SurfacePoint &SurfacePoint::operator=(const SurfacePoint &sp)
{
	if(this == &sp) return *this;
	if(sp.mat_data_) mat_data_ = sp.mat_data_->clone();
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

inline Vec3 SurfacePoint::normalFaceForward(const Vec3 &normal_geometry, const Vec3 &normal, const Vec3 &incoming_vector)
{
	return (normal_geometry * incoming_vector) < 0 ? -normal : normal;
}

inline const MaterialData * SurfacePoint::initBsdf(const Camera *camera)
{
	return primitive_->getMaterial()->initBsdf(*this, camera);
}

inline Rgb SurfacePoint::eval(const Vec3 &wo, const Vec3 &wl, const BsdfFlags &types, bool force_eval) const
{
	return primitive_->getMaterial()->eval(mat_data_.get(), *this, wo, wl, types, force_eval);
}

inline Rgb SurfacePoint::sample(const Vec3 &wo, Vec3 &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	return primitive_->getMaterial()->sample(mat_data_.get(), *this, wo, wi, s, w, chromatic, wavelength, camera);
}

inline Rgb SurfacePoint::sample(const Vec3 &wo, Vec3 *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const
{
	return primitive_->getMaterial()->sample(mat_data_.get(), *this, wo, dir, tcol, s, w, chromatic, wavelength);
}

inline float SurfacePoint::pdf(const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	return primitive_->getMaterial()->pdf(mat_data_.get(), *this, wo, wi, bsdfs);
}

inline Rgb SurfacePoint::getTransparency(const Vec3 &wo, const Camera *camera) const
{
	return primitive_->getMaterial()->getTransparency(mat_data_.get(), *this, wo, camera);
}

inline Specular SurfacePoint::getSpecular(int ray_level, const Vec3 &wo, bool chromatic, float wavelength) const
{
	return primitive_->getMaterial()->getSpecular(ray_level, mat_data_.get(), *this, wo, chromatic, wavelength);
}

inline Rgb SurfacePoint::getReflectivity(FastRandom &fast_random, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const
{
	return primitive_->getMaterial()->getReflectivity(fast_random, mat_data_.get(), *this, flags, chromatic, wavelength, camera);
}

inline Rgb SurfacePoint::emit(const Vec3 &wo) const
{
	return primitive_->getMaterial()->emit(mat_data_.get(), *this, wo);
}

inline float SurfacePoint::getAlpha(const Vec3 &wo, const Camera *camera) const
{
	return primitive_->getMaterial()->getAlpha(mat_data_.get(), *this, wo, camera);
}

inline bool SurfacePoint::scatterPhoton(const Vec3 &wi, Vec3 &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const
{
	return primitive_->getMaterial()->scatterPhoton(mat_data_.get(), *this, wi, wo, s, chromatic, wavelength, camera);
}

END_YAFARAY

#endif
