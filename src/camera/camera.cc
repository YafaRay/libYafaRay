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

#include "camera/camera.h"
#include "camera/camera_angular.h"
#include "camera/camera_architect.h"
#include "camera/camera_equirectangular.h"
#include "camera/camera_orthographic.h"
#include "camera/camera_perspective.h"
#include "common/param.h"
#include "common/logger.h"

namespace yafaray {

Camera::Params::Params(const ParamMap &param_map)
{
	loadParamMap(param_map);
}

void Camera::Params::loadParamMap(const ParamMap &param_map)
{
	param_map.getParam("type", type_);
	param_map.getParam("from", from_);
	param_map.getParam("to", to_);
	param_map.getParam("up", up_);
	param_map.getParam("resx", resolution_[Axis::X]);
	param_map.getParam("resy", resolution_[Axis::Y]);
	param_map.getParam("aspect_ratio", aspect_ratio_factor_);
	param_map.getParam("near_clip_distance", near_clip_distance_);
	param_map.getParam("far_clip_distance", far_clip_distance_);
}

ParamMap Camera::Params::getAsParamMap() const
{
	ParamMap param_map;
	param_map["type"] = type_;
	param_map["from"] = from_;
	param_map["to"] = to_;
	param_map["up"] = up_;
	param_map["resx"] = resolution_[Axis::X];
	param_map["resy"] = resolution_[Axis::Y];
	param_map["aspect_ratio_factor"] = aspect_ratio_factor_;
	param_map["near_clip_distance"] = near_clip_distance_;
	param_map["far_clip_distance"] = far_clip_distance_;
	return param_map;
}

ParamMap Camera::getAsParamMap() const
{
	return params_.getAsParamMap();
}

const Camera * Camera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Camera");
		param_map.logContents(logger);
	}
	std::string type;
	param_map.getParam("type", type);
	if(type == "angular") return AngularCamera::factory(logger, scene, name, param_map);
	else if(type == "perspective") return PerspectiveCamera::factory(logger, scene, name, param_map);
	else if(type == "architect") return ArchitectCamera::factory(logger, scene, name, param_map);
	else if(type == "orthographic") return OrthographicCamera::factory(logger, scene, name, param_map);
	else if(type == "equirectangular") return EquirectangularCamera::factory(logger, scene, name, param_map);
	else return nullptr;
}

Camera::Camera(Logger &logger, const Params &params) :
		params_{params},
		aspect_ratio_{params.aspect_ratio_factor_ * static_cast<float>(params.resolution_[Axis::Y]) / static_cast<float>(params.resolution_[Axis::X])},
		logger_{logger}
{
	// Calculate and store camera axis
	cam_y_ = params_.up_ - params_.from_;
	cam_z_ = params_.to_ - params_.from_;
	cam_x_ = cam_z_ ^ cam_y_;
	cam_y_ = cam_z_ ^ cam_x_;
	cam_x_.normalize();
	cam_y_.normalize();
	cam_z_.normalize();

	near_plane_.n_ = cam_z_;
	near_plane_.p_ = params_.from_ + cam_z_ * params_.near_clip_distance_;

	far_plane_.n_ = cam_z_;
	far_plane_.p_ = params_.from_ + cam_z_ * params_.far_clip_distance_;
}

} //namespace yafaray
