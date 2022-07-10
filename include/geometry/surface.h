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

#include "geometry/vector.h"
#include "geometry/object/object.h"
#include "geometry/primitive/primitive.h"
#include "color/color.h"
#include "material/material.h"
#include "math/interpolation.h"

namespace yafaray {
class Light;
class Object;
template<typename T> class Ray;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
class Primitive;
template<typename T> class RayDifferentials;

struct SurfaceDifferentials final
{
	SurfaceDifferentials() = default;
	SurfaceDifferentials(const SurfaceDifferentials &surface_differentials) = default;
	SurfaceDifferentials(SurfaceDifferentials &&surface_differentials) = default;
	SurfaceDifferentials(const Vec3f &dp_dx, const Vec3f &dp_dy) : dp_dx_(dp_dx), dp_dy_(dp_dy) { }
	Vec3f dp_dx_;
	Vec3f dp_dy_;
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
		explicit SurfacePoint(const Primitive *primitive) : primitive_(primitive) { }
		SurfacePoint(const SurfacePoint &sp);
		SurfacePoint(const SurfacePoint &sp_1, const SurfacePoint &sp_2, float alpha);
		SurfacePoint& operator=(const SurfacePoint &sp);
		SurfacePoint(SurfacePoint &&surface_point) = default;
		SurfacePoint& operator=(SurfacePoint&& surface_point) = default;
		[[nodiscard]] static Vec3f normalFaceForward(const Vec3f &normal_geometry, const Vec3f &normal, const Vec3f &incoming_vector);
		[[nodiscard]] float getDistToNearestEdge() const;
		//! compute differentials for a scattered ray
		[[nodiscard]] std::unique_ptr<RayDifferentials<float>> reflectedRay(const RayDifferentials<float> *in_differentials, const Vec3f &in_dir, const Vec3f &out_dir) const;
		//! compute differentials for a refracted ray
		[[nodiscard]] std::unique_ptr<RayDifferentials<float>> refractedRay(const RayDifferentials<float> *in_differentials, const Vec3f &in_dir, const Vec3f &out_dir, float ior) const;
		[[nodiscard]] float projectedPixelArea() const;
		[[nodiscard]] std::array<Uv<float>, 2> getUVdifferentialsXY() const;
		[[nodiscard]] std::unique_ptr<SurfaceDifferentials> calcSurfaceDifferentials(const RayDifferentials<float> *ray_differentials) const;

		[[nodiscard]] const MaterialData * initBsdf(const Camera *camera);
		[[nodiscard]] Rgb eval(const Vec3f &wo, const Vec3f &wl, BsdfFlags types, bool force_eval = false) const;
		[[nodiscard]] Rgb sample(const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const;
		[[nodiscard]] Rgb sample(const Vec3f &wo, Vec3f *dir, Rgb &tcol, Sample &s, float *w, bool chromatic, float wavelength) const;
		[[nodiscard]] float pdf(const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const;
		[[nodiscard]] Rgb getTransparency(const Vec3f &wo, const Camera *camera) const;
		[[nodiscard]] Specular getSpecular(int ray_level, const Vec3f &wo, bool chromatic, float wavelength) const;
		[[nodiscard]] Rgb getReflectivity(FastRandom &fast_random, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const;
		[[nodiscard]] Rgb emit(const Vec3f &wo) const;
		[[nodiscard]] float getAlpha(const Vec3f &wo, const Camera *camera) const;
		[[nodiscard]] bool scatterPhoton(const Vec3f &wi, Vec3f &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const;
		[[nodiscard]] unsigned int getObjectIndex() const { if(primitive_) return primitive_->getObjectIndex(); else return 0; }
		[[nodiscard]] unsigned int getObjectIndexAuto() const { if(primitive_) return primitive_->getObjectIndexAuto(); else return 0; }
		[[nodiscard]] Rgb getObjectIndexAutoColor() const { if(primitive_) return primitive_->getObjectIndexAutoColor(); else return Rgb{0.f}; }
		[[nodiscard]] const Material *getMaterial() const { if(primitive_) return primitive_->getMaterial(); else return nullptr; }
		[[nodiscard]] const Light *getLight() const { if(primitive_) return primitive_->getObjectLight(); else return nullptr; }
		[[nodiscard]] bool hasMotionBlur() const { if(primitive_) return primitive_->hasObjectMotionBlur(); else return false; }

		alignas(8) std::unique_ptr<const MaterialData> mat_data_;
		std::unique_ptr<const SurfaceDifferentials> differentials_; //!< Surface Differentials for mipmaps calculations

		// Geometry related
		Vec3f n_; //!< the shading normal.
		Vec3f ng_; //!< the geometric normal.
		Vec3f orco_ng_; //!< the untransformed geometric normal.
		Point3f p_; //!< the (world) position.
		Point3f orco_p_;
		//	Rgb vertex_col;
		bool has_uv_;
		bool has_orco_;
		float time_;

		Uv<float> uv_; //!< the u, v texture coords.
		Uv<Vec3f> uvn_; //!< vectors building orthogonal shading space with n_
		Uv<Vec3f> dp_; //!< u-axis and v-axis (dp/du, dp/dv) in world space (normalized)
		Uv<Vec3f> ds_; //!< u-axis and v-axis (ds/du, ds/dv) in shading space (uv_n_.u_, uv_n.v_, n_)
		Uv<Vec3f> dp_abs_; //!< u-axis and v-axis (dp/du, dp/dv) in world space  (before normalization)

	private:
		[[nodiscard]] static Uv<float> dUdvFromPointDifferentials(const Vec3f &dp, const Uv<Vec3f> &dp_duv);
		const Primitive *primitive_ = nullptr; //!< primitive the surface belongs to
};

inline SurfacePoint::SurfacePoint(const SurfacePoint &sp)
	:
	n_{sp.n_},
	ng_{sp.ng_},
	orco_ng_{sp.orco_ng_},
	p_{sp.p_},
	orco_p_{sp.orco_p_},
	has_uv_{sp.has_uv_},
	has_orco_{sp.has_orco_},
	time_{sp.time_},
	uv_{sp.uv_},
	uvn_{sp.uvn_},
	dp_{sp.dp_},
	ds_{sp.ds_},
	dp_abs_{sp.dp_abs_},
	primitive_{sp.primitive_}
{
	if(sp.mat_data_) mat_data_ = sp.mat_data_->clone();
	if(sp.differentials_) differentials_ = std::make_unique<const SurfaceDifferentials>(*sp.differentials_);
}

inline SurfacePoint::SurfacePoint(const SurfacePoint &sp_1, const SurfacePoint &sp_2, float alpha)
	:
	  n_{math::lerp(sp_1.n_, sp_2.n_, alpha)},
	  ng_{alpha < 0.5f ? sp_1.ng_ : sp_2.ng_},
	  orco_ng_{alpha < 0.5f ? sp_1.orco_ng_ : sp_2.orco_ng_},
	  p_{alpha < 0.5f ? sp_1.p_ : sp_2.p_},
	  orco_p_{alpha < 0.5f ? sp_1.orco_p_ : sp_2.orco_p_},
	  has_uv_{alpha < 0.5f ? sp_1.has_uv_ : sp_2.has_uv_},
	  has_orco_{alpha < 0.5f ? sp_1.has_orco_ : sp_2.has_orco_},
	  time_{alpha < 0.5f ? sp_1.time_ : sp_2.time_},
	  uv_{alpha < 0.5f ? sp_1.uv_ : sp_2.uv_},
	  uvn_{math::lerp(sp_1.uvn_, sp_2.uvn_, alpha)},
	  dp_{math::lerp(sp_1.dp_, sp_2.dp_, alpha)},
	  ds_{math::lerp(sp_1.ds_, sp_2.ds_, alpha)},
	  dp_abs_{alpha < 0.5f ? sp_1.dp_abs_ : sp_2.dp_abs_},
	  primitive_{alpha < 0.5f ? sp_1.primitive_ : sp_2.primitive_}
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
	n_ = sp.n_;
	ng_ = sp.ng_;
	orco_ng_ = sp.orco_ng_;
	p_ = sp.p_;
	orco_p_ = sp.orco_p_;
	has_uv_ = sp.has_uv_;
	has_orco_ = sp.has_orco_;
	time_ = sp.time_;
	uv_ = sp.uv_;
	uvn_ = sp.uvn_;
	dp_ = sp.dp_;
	ds_ = sp.ds_;
	dp_abs_ = sp.dp_abs_;
	if(sp.differentials_) differentials_ = std::make_unique<const SurfaceDifferentials>(*sp.differentials_);
	return *this;
}

inline Vec3f SurfacePoint::normalFaceForward(const Vec3f &normal_geometry, const Vec3f &normal, const Vec3f &incoming_vector)
{
	return (normal_geometry * incoming_vector) < 0 ? -normal : normal;
}

inline const MaterialData * SurfacePoint::initBsdf(const Camera *camera)
{
	return primitive_->getMaterial()->initBsdf(*this, camera);
}

inline Rgb SurfacePoint::eval(const Vec3f &wo, const Vec3f &wl, BsdfFlags types, bool force_eval) const
{
	return primitive_->getMaterial()->eval(mat_data_.get(), *this, wo, wl, types, force_eval);
}

inline Rgb SurfacePoint::sample(const Vec3f &wo, Vec3f &wi, Sample &s, float &w, bool chromatic, float wavelength, const Camera *camera) const
{
	return primitive_->getMaterial()->sample(mat_data_.get(), *this, wo, wi, s, w, chromatic, wavelength, camera);
}

inline Rgb SurfacePoint::sample(const Vec3f &wo, Vec3f *const dir, Rgb &tcol, Sample &s, float *const w, bool chromatic, float wavelength) const
{
	return primitive_->getMaterial()->sample(mat_data_.get(), *this, wo, dir, tcol, s, w, chromatic, wavelength);
}

inline float SurfacePoint::pdf(const Vec3f &wo, const Vec3f &wi, BsdfFlags bsdfs) const
{
	return primitive_->getMaterial()->pdf(mat_data_.get(), *this, wo, wi, bsdfs);
}

inline Rgb SurfacePoint::getTransparency(const Vec3f &wo, const Camera *camera) const
{
	return primitive_->getMaterial()->getTransparency(mat_data_.get(), *this, wo, camera);
}

inline Specular SurfacePoint::getSpecular(int ray_level, const Vec3f &wo, bool chromatic, float wavelength) const
{
	return primitive_->getMaterial()->getSpecular(ray_level, mat_data_.get(), *this, wo, chromatic, wavelength);
}

inline Rgb SurfacePoint::getReflectivity(FastRandom &fast_random, BsdfFlags flags, bool chromatic, float wavelength, const Camera *camera) const
{
	return primitive_->getMaterial()->getReflectivity(fast_random, mat_data_.get(), *this, flags, chromatic, wavelength, camera);
}

inline Rgb SurfacePoint::emit(const Vec3f &wo) const
{
	return primitive_->getMaterial()->emit(mat_data_.get(), *this, wo);
}

inline float SurfacePoint::getAlpha(const Vec3f &wo, const Camera *camera) const
{
	return primitive_->getMaterial()->getAlpha(mat_data_.get(), *this, wo, camera);
}

inline bool SurfacePoint::scatterPhoton(const Vec3f &wi, Vec3f &wo, PSample &s, bool chromatic, float wavelength, const Camera *camera) const
{
	return primitive_->getMaterial()->scatterPhoton(mat_data_.get(), *this, wi, wo, s, chromatic, wavelength, camera);
}

} //namespace yafaray

#endif
