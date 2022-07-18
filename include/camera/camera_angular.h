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
		struct Params
		{
			Params(const ParamMap &param_map);
			ParamMap getAsParamMap() const;
			enum class Projection : unsigned char  //Fish Eye Projections as defined in https://en.wikipedia.org/wiki/Fisheye_lens
			{
				Equidistant, //!<Default and used traditionally in YafaRay
				Orthographic, //!<Orthographic projection where the centre of the image is enlarged/more defined at the cost of much more distorted edges. Angle should be 90º or less
				Stereographic, //!<angle should be less than 180º
				EquisolidAngle,
				Rectilinear, //!<angle should be less than 90º
			};
			Projection projection_ = Projection::Equidistant;
			float angle_degrees_ = 90.f;
			float max_angle_degrees_ = 90;
			bool circular_ = true;
			bool mirrored_ = false;
		};
		static const Camera * factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);

	private:
		AngularCamera(Logger &logger, const Camera::Params &camera_params, const Params &params);
		ParamMap getAsParamMap() const override;
		void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz) override;
		CameraRay shootRay(float px, float py, const Uv<float> &uv) const override;
		Point3f screenproject(const Point3f &p) const override;

		Params params_;
		float focal_length_;
		float angle_;
		float max_radius_;
};


} //namespace yafaray

#endif // YAFARAY_CAMERA_ANGULAR_H
