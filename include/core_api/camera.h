#pragma once
/****************************************************************************
 *
 * 			camera.h: Camera implementation api
 *      This is part of the yafray package
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

#include <yafray_constants.h>
#include <utilities/geometry.h>

BEGIN_YAFRAY

class Vec3;
class Point3;

//! Camera base class.
/*!
 Camera base class used by all camera types.
*/
class YAFRAYCORE_EXPORT Camera
{
	public:
		Camera() = default;
		Camera(const Point3 &pos, const Point3 &look, const Point3 &up, int resx, int resy, float aspect, float const near_clip_distance, float const far_clip_distance) :
				position_(pos), resx_(resx), resy_(resy), aspect_ratio_(aspect * (float)resy_ / (float)resx_), camera_name_(""), view_name_("")
		{
			// Calculate and store camera axis
			cam_y_ = up - position_;
			cam_z_ = look - position_;
			cam_x_ = cam_z_ ^ cam_y_;
			cam_y_ = cam_z_ ^ cam_x_;
			cam_x_.normalize();
			cam_y_.normalize();
			cam_z_.normalize();

			near_plane_.n_ = cam_z_;
			near_plane_.p_ = Vec3(position_) + cam_z_ * near_clip_distance;

			far_plane_.n_ = cam_z_;
			far_plane_.p_ = Vec3(position_) + cam_z_ * far_clip_distance;

			near_clip_ = near_clip_distance;
			far_clip_ = far_clip_distance;
		}
		virtual ~Camera() {}
		virtual void setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz) = 0; //!< Set camera axis
		/*! Shoot a new ray from the camera gived image pixel coordinates px,py and lense dof effect */
		virtual Ray shootRay(float px, float py, float u, float v, float &wt) const = 0; //!< Shoot a new ray from the camera.
		virtual Point3 screenproject(const Point3 &p) const = 0; //!< Get projection of point p into camera plane

		virtual int resX() const { return resx_; } //!< Get camera X resolution
		virtual int resY() const { return resy_; } //!< Get camera Y resolution
		virtual Point3 getPosition() const { return position_; } //!< Get camera position
		virtual void setPosition(const Point3 &pos) { position_ = pos; } //!< Set camera position
		virtual void getAxis(Vec3 &vx, Vec3 &vy, Vec3 &vz) const { vx = cam_x_; vy = cam_y_; vz = cam_z_; } //!< Get camera axis
		/*! Indicate whether the lense need to be sampled (u, v parameters of shootRay), i.e.
			DOF-like effects. When false, no lense samples need to be computed */
		virtual bool sampleLense() const { return false; } //!< Indicate whether the lense need to be sampled
		virtual bool project(const Ray &wo, float lu, float lv, float &u, float &v, float &pdf) const { return false; }
		virtual float getNearClip() const { return near_clip_; }
		virtual float getFarClip() const { return far_clip_; }
		void setCameraName(std::string name) { camera_name_ = name; }
		std::string getCameraName() const { return camera_name_; }
		std::string getViewName() const { return view_name_; }

	protected:
		Point3 position_;	//!< Camera position

		int resx_;		//!< Camera X resolution
		int resy_;		//!< Camera Y resolution

		Vec3 cam_x_;	//!< Camera X axis
		Vec3 cam_y_;	//!< Camera Y axis
		Vec3 cam_z_;	//!< Camera Z axis

		Vec3 vto_;
		Vec3 vup_;
		Vec3 vright_;

		float aspect_ratio_;	//<! Aspect ratio of camera (not image in pixel units!)
		std::string camera_name_;       //<! Camera name
		std::string view_name_;       //<! View name for file saving and Blender MultiView environment

		Plane near_plane_, far_plane_;
		float near_clip_, far_clip_;
};

END_YAFRAY

#endif // YAFARAY_CAMERA_H
