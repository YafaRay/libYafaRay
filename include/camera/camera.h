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

#ifndef YAFARAY_CAMERA_H
#define YAFARAY_CAMERA_H

#include <memory>
#include <utility>
#include "geometry/plane.h"

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
	public:
		struct Params
		{
			Params() = default;
			Params(const ParamMap &param_map);
			void loadParamMap(const ParamMap &param_map);
			ParamMap getAsParamMap() const;
			Point3f from_{{0, 1, 0}}; //!< Camera position
			Point3f to_{{0, 0, 0}};
			Point3f up_{{0, 1, 1}};
			Size2i resolution_{{320, 200}}; //!< Camera resolution
			float aspect_ratio_factor_ = 1.f;
			float near_clip_distance_ = 0.f;
			float far_clip_distance_ = -1.f;
			std::string type_;
		};
		static const Camera * factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		Camera(Logger &logger, const Params &params);
		virtual ~Camera() = default;
		virtual ParamMap getAsParamMap() const;
		virtual void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz) = 0; //!< Set camera axis
		/*! Shoot a new ray from the camera gived image pixel coordinates px,py and lense dof effect */
		virtual CameraRay shootRay(float px, float py, const Uv<float> &uv) const = 0; //!< Shoot a new ray from the camera.
		virtual Point3f screenproject(const Point3f &p) const = 0; //!< Get projection of point p into camera plane
		virtual bool sampleLense() const { return false; } //!< Indicate whether the lense need to be sampled
		virtual bool project(const Ray &wo, float lu, float lv, float &u, float &v, float &pdf) const { return false; }
		int resX() const { return params_.resolution_[Axis::X]; } //!< Get camera X resolution
		int resY() const { return params_.resolution_[Axis::Y]; } //!< Get camera Y resolution
		Point3f getPosition() const { return params_.from_; } //!< Get camera position
		std::array<Vec3f, 3> getAxes() const { return {cam_x_, cam_y_, cam_z_}; } //!< Get camera axis
		/*! Indicate whether the lense need to be sampled (u, v parameters of shootRay), i.e.
			DOF-like effects. When false, no lense samples need to be computed */
		float getNearClip() const { return params_.near_clip_distance_; }
		float getFarClip() const { return params_.far_clip_distance_; }

	protected:
		const Params params_;
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

#endif // YAFARAY_CAMERA_H
