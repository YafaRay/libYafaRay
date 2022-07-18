/****************************************************************************
 *
 *      camera.cc: Camera implementation
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

#include "camera/camera_angular.h"

#include <cmath>
#include "common/param.h"

namespace yafaray {

AngularCamera::Params::Params(const ParamMap &param_map)
{
	std::string projection_string;

	param_map.getParam("angle", angle_degrees_);
	max_angle_degrees_ = angle_degrees_;
	param_map.getParam("max_angle", max_angle_degrees_);
	param_map.getParam("circular", circular_);
	param_map.getParam("mirrored", mirrored_);
	param_map.getParam("projection", projection_string);

	if(projection_string == "orthographic") projection_ = Params::Projection::Orthographic;
	else if(projection_string == "stereographic") projection_ = Params::Projection::Stereographic;
	else if(projection_string == "equisolid_angle") projection_ = Params::Projection::EquisolidAngle;
	else if(projection_string == "rectilinear") projection_ = Params::Projection::Rectilinear;
	else projection_ = Params::Projection::Equidistant;
}

ParamMap AngularCamera::Params::getAsParamMap() const
{
	ParamMap result;
	result["angle"] = angle_degrees_;
	result["max_angle"] = max_angle_degrees_;
	result["circular"] = circular_;
	result["mirrored"] = mirrored_;
	std::string projection_string;
	switch(projection_)
	{
		case Params::Projection::Orthographic: projection_string = "orthographic"; break;
		case Params::Projection::Stereographic: projection_string = "stereographic"; break;
		case Params::Projection::EquisolidAngle: projection_string = "equisolid_angle"; break;
		case Params::Projection::Rectilinear: projection_string = "rectilinear"; break;
		default: projection_string = "equidistant"; break;
	}
	result["projection"] = projection_string;
	return result;
}

ParamMap AngularCamera::getAsParamMap() const
{
	ParamMap result{Camera::params_.getAsParamMap()};
	result.append(params_.getAsParamMap());
	return result;
}

const Camera * AngularCamera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	Camera::Params camera_params;
	camera_params.far_clip_distance_ = -1.0e38f;
	camera_params.loadParamMap(param_map);
	Params params{param_map};
	auto cam = new AngularCamera(logger, camera_params, params);
	if(params.mirrored_) cam->vright_ = -cam->vright_;
	return cam;
}

AngularCamera::AngularCamera(Logger &logger, const Camera::Params &camera_params, const Params &params) : Camera{logger, camera_params},
		params_{params}, angle_{params.angle_degrees_ * math::div_pi_by_180<>}, max_radius_{params.max_angle_degrees_ / params.angle_degrees_}
{
	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);

	if(params_.projection_ == Params::Projection::Orthographic) focal_length_ = 1.f / math::sin(angle_);
	else if(params_.projection_ == Params::Projection::Stereographic) focal_length_ = 1.f / 2.f / std::tan(angle_ / 2.f);
	else if(params_.projection_ == Params::Projection::EquisolidAngle) focal_length_ = 1.f / 2.f / math::sin(angle_ / 2.f);
	else if(params_.projection_ == Params::Projection::Rectilinear) focal_length_ = 1.f / std::tan(angle_);
	else focal_length_ = 1.f / angle_; //By default, AngularProjection::Equidistant
}

void AngularCamera::setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	vright_ = cam_x_;
	vup_ = cam_y_;
	vto_ = cam_z_;
}

CameraRay AngularCamera::shootRay(float px, float py, const Uv<float> &uv) const
{
	const float u = 1.f - 2.f * (px / static_cast<float>(resX()));
	float v = 2.f * (py / static_cast<float>(resY())) - 1.f;
	v *= aspect_ratio_;
	const float radius = math::sqrt(u * u + v * v);
	Ray ray;
	ray.from_ = Camera::params_.from_;
	if(params_.circular_ && radius > max_radius_) { return {std::move(ray), false}; }
	float theta = 0.f;
	if(!((u == 0.f) && (v == 0.f))) theta = std::atan2(v, u);
	float phi;
	if(params_.projection_ == Params::Projection::Orthographic) phi = math::asin(radius / focal_length_);
	else if(params_.projection_ == Params::Projection::Stereographic) phi = 2.f * std::atan(radius / (2.f * focal_length_));
	else if(params_.projection_ == Params::Projection::EquisolidAngle) phi = 2.f * math::asin(radius / (2.f * focal_length_));
	else if(params_.projection_ == Params::Projection::Rectilinear) phi = std::atan(radius / focal_length_);
	else phi = radius / focal_length_; //By default, AngularProjection::Equidistant
	//float sp = sin(phi);
	ray.dir_ = math::sin(phi) * (math::cos(theta) * vright_ + math::sin(theta) * vup_) + math::cos(phi) * vto_;
	ray.tmin_ = near_plane_.rayIntersection(ray);
	ray.tmax_ = far_plane_.rayIntersection(ray);
	return {std::move(ray), true};
}

Point3f AngularCamera::screenproject(const Point3f &p) const
{
	//FIXME
	Vec3f dir{p - Camera::params_.from_};
	dir.normalize();
	// project p to pixel plane:
	const float dx = cam_x_ * dir;
	const float dy = cam_y_ * dir;
	const float dz = cam_z_ * dir;
	return {{ -dx / (4.f * math::num_pi<> * dz), -dy / (4.f * math::num_pi<> * dz), 0.f }};
}

} //namespace yafaray
