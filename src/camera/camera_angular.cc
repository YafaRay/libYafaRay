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
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

AngularCamera::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(angle_degrees_);
	max_angle_degrees_ = angle_degrees_;
	PARAM_LOAD(max_angle_degrees_);
	PARAM_LOAD(circular_);
	PARAM_LOAD(mirrored_);
	PARAM_ENUM_LOAD(projection_);
}

ParamMap AngularCamera::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(angle_degrees_);
	PARAM_SAVE(max_angle_degrees_);
	PARAM_SAVE(circular_);
	PARAM_SAVE(mirrored_);
	PARAM_ENUM_SAVE(projection_);
	PARAM_SAVE_END;
}

ParamMap AngularCamera::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Camera::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Camera *, ParamError> AngularCamera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{

	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new AngularCamera(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<AngularCamera>(name, {"type"}));
	return {result, param_error};
}

AngularCamera::AngularCamera(Logger &logger, ParamError &param_error, const ParamMap &param_map) : Camera{logger, param_error, param_map},
		params_{param_error, param_map}, angle_{params_.angle_degrees_ * math::div_pi_by_180<>}, max_radius_{params_.max_angle_degrees_ / params_.angle_degrees_}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	if(params_.mirrored_) vright_ = -vright_;
	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);
	switch(params_.projection_.value())
	{
		case Projection::Orthographic: focal_length_ = 1.f / math::sin(angle_); break;
		case Projection::Stereographic: focal_length_ = 1.f / 2.f / std::tan(angle_ / 2.f); break;
		case Projection::EquisolidAngle: focal_length_ = 1.f / 2.f / math::sin(angle_ / 2.f); break;
		case Projection::Rectilinear: focal_length_ = 1.f / std::tan(angle_); break;
		case Projection::Equidistant:
		default: focal_length_ = 1.f / angle_; break;
	}
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
	switch(params_.projection_.value())
	{
		case Projection::Orthographic: phi = math::asin(radius / focal_length_); break;
		case Projection::Stereographic: phi = 2.f * std::atan(radius / (2.f * focal_length_)); break;
		case Projection::EquisolidAngle: phi = 2.f * math::asin(radius / (2.f * focal_length_)); break;
		case Projection::Rectilinear: phi = std::atan(radius / focal_length_); break;
		case Projection::Equidistant:
		default: phi = radius / focal_length_; break;
	}
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
