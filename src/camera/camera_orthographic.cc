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

#include "camera/camera_orthographic.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

OrthographicCamera::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(scale_);
}

ParamMap OrthographicCamera::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(scale_);
	PARAM_SAVE_END;
}

ParamMap OrthographicCamera::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Camera>, ParamError> OrthographicCamera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto camera {std::make_unique<ThisClassType_t>(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	return {std::move(camera), param_error};
}

OrthographicCamera::OrthographicCamera(Logger &logger, ParamError &param_error, const ParamMap &param_map) : ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);
}

void OrthographicCamera::setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	vright_ = cam_x_;
	vup_ = aspect_ratio_ * cam_y_;
	vto_ = cam_z_;
	pos_ = ParentClassType_t::params_.from_ - 0.5f * params_.scale_ * (vup_ + vright_);
	vup_     *= params_.scale_ / static_cast<float>(resY());
	vright_  *= params_.scale_ / static_cast<float>(resX());
}


CameraRay OrthographicCamera::shootRay(float px, float py, const Uv<float> &uv) const
{
	Ray ray;
	ray.from_ = pos_ + vright_ * px + vup_ * py;
	ray.dir_ = vto_;
	ray.tmin_ = near_plane_.rayIntersection(ray);
	ray.tmax_ = far_plane_.rayIntersection(ray);
	return {std::move(ray), true};
}

Point3f OrthographicCamera::screenproject(const Point3f &p) const
{
	const Vec3f dir{p - pos_};
	// Project p to pixel plane
	const float dz = cam_z_ * dir;
	const Vec3f proj{dir - dz * cam_z_};
	return {{ 2.f * (proj * cam_x_ / params_.scale_) - 1.f, -2.f * proj * cam_y_ / (aspect_ratio_ * params_.scale_) + 1.f, 0.f }};
}

} //namespace yafaray
