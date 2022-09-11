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

#include "camera/camera_perspective.h"
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

PerspectiveCamera::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(focal_distance_);
	PARAM_LOAD(aperture_);
	PARAM_LOAD(depth_of_field_distance_);
	PARAM_ENUM_LOAD(bokeh_type_);
	PARAM_ENUM_LOAD(bokeh_bias_);
	PARAM_LOAD(bokeh_rotation_);
}

ParamMap PerspectiveCamera::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(focal_distance_);
	PARAM_SAVE(aperture_);
	PARAM_SAVE(depth_of_field_distance_);
	PARAM_SAVE(bokeh_rotation_);
	PARAM_ENUM_SAVE(bokeh_type_);
	PARAM_ENUM_SAVE(bokeh_bias_);
	PARAM_SAVE_END;
}

ParamMap PerspectiveCamera::getAsParamMap(bool only_non_default) const
{
	ParamMap result{Camera::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<Camera *, ParamError> PerspectiveCamera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	auto result {new PerspectiveCamera(logger, param_error, param_map)};
	if(param_error.flags_ != ParamError::Flags::Ok) logger.logWarning(param_error.print<PerspectiveCamera>(name, {"type"}));
	return {result, param_error};
}

PerspectiveCamera::PerspectiveCamera(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		Camera{logger, param_error, param_map}, params_{param_error, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());

	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);

	fdist_ = (Camera::params_.to_ - Camera::params_.from_).length();
	a_pix_ = aspect_ratio_ / (params_.focal_distance_ * params_.focal_distance_);

	int ns = params_.bokeh_type_.value();
	if((ns >= 3) && (ns <= 6))
	{
		float w = math::degToRad(params_.bokeh_rotation_), wi = math::mult_pi_by_2<> / static_cast<float>(ns);
		ns = (ns + 2) * 2;
		ls_.resize(ns);
		for(int i = 0; i < ns; i += 2)
		{
			ls_[i] = math::cos(w);
			ls_[i + 1] = math::sin(w);
			w += wi;
		}
	}
}

void PerspectiveCamera::setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	dof_rt_ = params_.aperture_ * cam_x_; // for dof, premul with aperture
	dof_up_ = params_.aperture_ * cam_y_;

	vright_ = cam_x_;
	vup_ = aspect_ratio_ * cam_y_;
	vto_ = (cam_z_ * params_.focal_distance_) - 0.5f * (vup_ + vright_);
	vup_ /= static_cast<float>(resY());
	vright_ /= static_cast<float>(resX());
}

float PerspectiveCamera::biasDist(float r) const
{
	switch(params_.bokeh_bias_.value())
	{
		case BokehBias::Center:
			return math::sqrt(math::sqrt(r) * r);
		case BokehBias::Edge:
			return math::sqrt(1.f - r * r);
		default:
		case BokehBias::None:
			return math::sqrt(r);
	}
}

Uv<float> PerspectiveCamera::sampleTsd(float r_1, float r_2) const
{
	const auto fn = static_cast<float>(params_.bokeh_type_.value());
	int idx = static_cast<int>(r_1 * fn);
	float r = (r_1 - (static_cast<float>(idx)) / fn) * fn;
	r = biasDist(r);
	const float b_1 = r * r_2;
	const float b_0 = r - b_1;
	idx <<= 1;
	const Uv<float> uv{
			ls_[idx] * b_0 + ls_[idx + 2] * b_1,
			ls_[idx + 1] * b_0 + ls_[idx + 3] * b_1
	};
	return uv;
}

Uv<float> PerspectiveCamera::getLensUv(float r_1, float r_2) const
{
	switch(params_.bokeh_type_.value())
	{
		case BokehType::Triangle:
		case BokehType::Square:
		case BokehType::Pentagon:
		case BokehType::Hexagon: return sampleTsd(r_1, r_2);
		case BokehType::Disk2:
		case BokehType::Ring:
		{
			const float w = math::mult_pi_by_2<> * r_2;
			const float r = (params_.bokeh_type_ == BokehType::Ring) ?
					math::sqrt(0.707106781f + 0.292893218f) :
					biasDist(r_1);
			return {r * math::cos(w), r * math::sin(w)};
		}
		default:
		case BokehType::Disk1: return Vec3f::shirleyDisk(r_1, r_2);
	}
}

CameraRay PerspectiveCamera::shootRay(float px, float py, const Uv<float> &uv) const
{
	Ray ray{Camera::params_.from_, vright_ * px + vup_ * py + vto_, 0.f};
	ray.dir_.normalize();
	ray.tmin_ = near_plane_.rayIntersection(ray);
	ray.tmax_ = far_plane_.rayIntersection(ray);
	if(params_.aperture_ != 0.f)
	{
		const Uv<float> lens_uv{getLensUv(uv.u_, uv.v_)};
		const Vec3f li{dof_rt_ * lens_uv.u_ + dof_up_ * lens_uv.v_};
		ray.from_ += li;
		ray.dir_ = ray.dir_ * params_.depth_of_field_distance_ - li;
		ray.dir_.normalize();
	}
	return {std::move(ray), true};
}

Point3f PerspectiveCamera::screenproject(const Point3f &p) const
{
	const Vec3f dir{p - Camera::params_.from_};
	// project p to pixel plane:
	const float dx = dir * cam_x_;
	const float dy = dir * cam_y_;
	const float dz = dir * cam_z_;
	return {{ 2.f * dx * params_.focal_distance_ / dz, -2.f * dy * params_.focal_distance_ / (dz * aspect_ratio_), 0.f }};
}

bool PerspectiveCamera::project(const Ray &wo, float lu, float lv, float &u, float &v, float &pdf) const
{
	// project wo to pixel plane:
	const float dx = cam_x_ * wo.dir_;
	const float dy = cam_y_ * wo.dir_;
	const float dz = cam_z_ * wo.dir_;
	if(dz <= 0) return false;
	u = dx * params_.focal_distance_ / dz;
	if(u < -0.5 || u > 0.5) return false;
	u = (u + 0.5f) * static_cast<float>(resX());

	v = dy * params_.focal_distance_ / (dz * aspect_ratio_);
	if(v < -0.5 || v > 0.5) return false;
	v = (v + 0.5f) * static_cast<float>(resY());

	// pdf = 1/A_pix * r^2 / cos(forward, dir), where r^2 is also 1/cos(vto, dir)^2
	const float cos_wo = dz; //camZ * wo.dir;
	pdf = 8.f * math::num_pi<> / (a_pix_ * cos_wo * cos_wo * cos_wo);
	return true;
}

bool PerspectiveCamera::sampleLense() const { return params_.aperture_ != 0; }

} //namespace yafaray
