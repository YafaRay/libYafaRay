/****************************************************************************
 *
 *      equirectangularCamera.cc: Camera implementation
 *      This is part of the libYafaRay package
 *      Copyright (C) 2020  David Bluecame
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

#include "camera/camera_equirectangular.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

std::pair<std::unique_ptr<Camera>, ParamError> EquirectangularCamera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto camera {std::make_unique<ThisClassType_t>(logger, param_error, param_map)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	return {std::move(camera), param_error};
}

EquirectangularCamera::EquirectangularCamera(Logger &logger, ParamError &param_error, const ParamMap &param_map) : ParentClassType_t{logger, param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);
}

void EquirectangularCamera::setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	vright_ = cam_x_;
	vup_ = cam_y_;
	vto_ = cam_z_;
}

CameraRay EquirectangularCamera::shootRay(float px, float py, const Uv<float> &uv) const
{
	Ray ray;
	ray.from_ = ParentClassType_t::params_.from_;
	float u = 2.f * px / static_cast<float>(resX()) - 1.f;
	float v = 2.f * py / static_cast<float>(resY()) - 1.f;
	const float phi = math::num_pi<> * u;
	const float theta = math::div_pi_by_2<> * v;
	ray.dir_ = math::cos(theta) * (math::cos(phi) * vto_ + math::sin(phi) * vright_) + math::sin(theta) * vup_;
	ray.tmin_ = near_plane_.rayIntersection(ray);
	ray.tmax_ = far_plane_.rayIntersection(ray);
	return {std::move(ray), true};
}

Point3f EquirectangularCamera::screenproject(const Point3f &p) const
{
	//FIXME
	Vec3f dir{p - ParentClassType_t::params_.from_};
	dir.normalize();

	// project p to pixel plane:
	float dx = cam_x_ * dir;
	float dy = cam_y_ * dir;
	float dz = cam_z_ * dir;

	return {{-dx / (4.f * math::num_pi<> * dz), dy / (4.f * math::num_pi<> * dz), 0.f}};
}

} //namespace yafaray
