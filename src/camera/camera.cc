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
#include "param/param.h"
#include "common/logger.h"

namespace yafaray {

Camera::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(from_);
	PARAM_LOAD(to_);
	PARAM_LOAD(up_);
	PARAM_LOAD(resx_);
	PARAM_LOAD(resy_);
	PARAM_LOAD(aspect_ratio_factor_);
	PARAM_LOAD(near_clip_distance_);
	PARAM_LOAD(far_clip_distance_);
}

ParamMap Camera::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(from_);
	PARAM_SAVE(to_);
	PARAM_SAVE(up_);
	PARAM_SAVE(resx_);
	PARAM_SAVE(resy_);
	PARAM_SAVE(aspect_ratio_factor_);
	PARAM_SAVE(near_clip_distance_);
	PARAM_SAVE(far_clip_distance_);
	PARAM_SAVE_END;
}

ParamMap Camera::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<std::unique_ptr<Camera>, ParamError> Camera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Angular: return AngularCamera::factory(logger, scene, name, param_map);
		case Type::Perspective: return PerspectiveCamera::factory(logger, scene, name, param_map);
		case Type::Architect: return ArchitectCamera::factory(logger, scene, name, param_map);
		case Type::Orthographic: return OrthographicCamera::factory(logger, scene, name, param_map);
		case Type::Equirectangular: return EquirectangularCamera::factory(logger, scene, name, param_map);
		default: return {nullptr, {ParamError::Flags::ErrorWhileCreating}};
	}
}

Camera::Camera(Logger &logger, ParamError &param_error, const ParamMap &param_map) :
		params_{param_error, param_map},
		aspect_ratio_{params_.aspect_ratio_factor_ * static_cast<float>(params_.resy_) / static_cast<float>(params_.resx_)},
		logger_{logger}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());

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
