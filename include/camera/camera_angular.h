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

#ifndef YAFARAY_CAMERA_ANGULAR_H
#define YAFARAY_CAMERA_ANGULAR_H

#include "camera/camera.h"

namespace yafaray {

class ParamMap;
class Scene;

class AngularCamera final : public Camera
{
	public:
		static const Camera * factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		enum class Projection : unsigned char  //Fish Eye Projections as defined in https://en.wikipedia.org/wiki/Fisheye_lens
		{
				Equidistant, //Default and used traditionally in YafaRay
				Orthographic, //Orthographic projection where the centre of the image is enlarged/more defined at the cost of much more distorted edges
				Stereographic,
				EquisolidAngle,
				Rectilinear,
		};
		AngularCamera(Logger &logger, const Point3 &pos, const Point3 &look, const Point3 &up,
					  int resx, int resy, float aspect, float angle, float max_angle, bool circ, const Projection &projection,
					  float near_clip_distance = 0.0f, float far_clip_distance = 1e6f);
		void setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz) override;
		CameraRay shootRay(float px, float py, const Uv<float> &uv) const override;
		Point3 screenproject(const Point3 &p) const override;

		float focal_length_;
		float max_radius_;
		bool circular_;
		Projection projection_;
};


} //namespace yafaray

#endif // YAFARAY_CAMERA_ANGULAR_H
