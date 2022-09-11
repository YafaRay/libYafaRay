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

#include "camera/camera_architect.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

std::pair<Camera *, ParamError> ArchitectCamera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new ArchitectCamera(logger, param_error, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<ArchitectCamera>(name, {"type"}));
	return {result, param_error};
}

ArchitectCamera::ArchitectCamera(Logger &logger, ParamError &param_error, const ParamMap &param_map)
	: PerspectiveCamera(logger, param_error, param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
}

void ArchitectCamera::setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	dof_rt_ = cam_x_ * params_.aperture_; // for dof, premul with aperture
	dof_up_ = cam_y_ * params_.aperture_;

	vright_ = cam_x_;
	vup_ = aspect_ratio_ * Vec3f{{0.f, 0.f, -1.f}};
	vto_ = (cam_z_ * params_.focal_distance_) - 0.5f * (vup_ + vright_);
	vup_ /= static_cast<float>(resY());
	vright_ /= static_cast<float>(resX());
}

Point3f ArchitectCamera::screenproject(const Point3f &p) const
{
	// FIXME
	const Vec3f dir{p - Camera::params_.from_};
	// project p to pixel plane:
	const Vec3f camy{{0.f, 0.f, 1.f}};
	const Vec3f camz{camy ^ cam_x_};
	const Vec3f camx{camz ^ camy};

	const float dx = dir * camx;
	const float dy = dir * cam_y_;
	float dz = dir * camz;

	Point3f s;
	s[Axis::Y] = 2 * dy * params_.focal_distance_ / (dz * aspect_ratio_);
	// Needs focal_distance correction
	const float fod = params_.focal_distance_ * camy * cam_y_ / (camx * cam_x_);
	s[Axis::X] = 2 * dx * fod / dz;
	s[Axis::Z] = 0.f;
	return s;
}

} //namespace yafaray
