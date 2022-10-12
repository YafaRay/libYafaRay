#pragma once
/****************************************************************************
 *
 *      camera.h: Camera implementation api
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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

#ifndef LIBYAFARAY_CAMERA_H
#define LIBYAFARAY_CAMERA_H

#include "geometry/plane.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"

namespace yafaray {

class ParamMap;
class Scene;

template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;
class Logger;

//! Camera base class.
/*!
 Camera base class used by all camera types.
*/

struct CameraRay
{
	CameraRay(Ray &&ray, bool valid) : ray_(std::move(ray)), valid_(valid) { }
	Ray ray_;
	bool valid_;
};

class Camera
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "Camera"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<Camera>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		Camera(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		virtual ~Camera() = default;
		virtual void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz) = 0; //!< Set camera axis
		/*! Shoot a new ray from the camera gived image pixel coordinates px,py and lense dof effect */
		virtual CameraRay shootRay(float px, float py, const Uv<float> &uv) const = 0; //!< Shoot a new ray from the camera.
		virtual Point3f screenproject(const Point3f &p) const = 0; //!< Get projection of point p into camera plane
		virtual bool sampleLense() const { return false; } //!< Indicate whether the lense need to be sampled
		virtual bool project(const Ray &wo, float lu, float lv, float &u, float &v, float &pdf) const { return false; }
		int resX() const { return params_.resx_; } //!< Get camera X resolution
		int resY() const { return params_.resy_; } //!< Get camera Y resolution
		Point3f getPosition() const { return params_.from_; } //!< Get camera position
		std::array<Vec3f, 3> getAxes() const { return {cam_x_, cam_y_, cam_z_}; } //!< Get camera axis
		/*! Indicate whether the lense need to be sampled (u, v parameters of shootRay), i.e.
			DOF-like effects. When false, no lense samples need to be computed */
		float getNearClip() const { return params_.near_clip_distance_; }
		float getFarClip() const { return params_.far_clip_distance_; }

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, Angular, Perspective, Architect, Orthographic, Equirectangular };
			inline static const EnumMap<ValueType_t> map_{{
					{"angular", Angular, ""},
					{"perspective", Perspective, ""},
					{"architect", Architect, ""},
					{"orthographic", Orthographic, ""},
					{"equirectangular", Equirectangular, ""},
				}};
		};
		const struct Params
		{
			PARAM_INIT;
			PARAM_DECL(Vec3f, from_, (Vec3f{{0, 1, 0}}), "from", "");
			PARAM_DECL(Vec3f, to_, (Vec3f{{0, 0, 0}}), "to", "");
			PARAM_DECL(Vec3f, up_, (Vec3f{{0, 1, 1}}), "up", "");
			PARAM_DECL(int, resx_, 320, "resx", "Camera resolution X");
			PARAM_DECL(int, resy_, 200, "resy", "Camera resolution Y");
			PARAM_DECL(float, aspect_ratio_factor_, 1.f, "aspect_ratio_factor", "");
			PARAM_DECL(float, near_clip_distance_, 0.f, "nearClip", "");
			PARAM_DECL(float, far_clip_distance_, -1.f, "farClip", "");
		} params_;
		Vec3f cam_x_;	//!< Camera X axis
		Vec3f cam_y_;	//!< Camera Y axis
		Vec3f cam_z_;	//!< Camera Z axis
		Vec3f vto_;
		Vec3f vup_;
		Vec3f vright_;
		float aspect_ratio_;	//<! Aspect ratio of camera (not image in pixel units!)
		Plane near_plane_, far_plane_;
		Logger &logger_;
};


} //namespace yafaray

#endif // LIBYAFARAY_CAMERA_H
