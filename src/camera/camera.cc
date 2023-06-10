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

std::map<std::string, const ParamMeta *> Camera::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(from_);
	PARAM_META(to_);
	PARAM_META(up_);
	PARAM_META(resx_);
	PARAM_META(resy_);
	PARAM_META(aspect_ratio_factor_);
	PARAM_META(near_clip_distance_);
	PARAM_META(far_clip_distance_);
	return param_meta_map;
}

Camera::Params::Params(ParamResult &param_result, const ParamMap &param_map)
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

ParamMap Camera::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	PARAM_SAVE(from_);
	PARAM_SAVE(to_);
	PARAM_SAVE(up_);
	PARAM_SAVE(resx_);
	PARAM_SAVE(resy_);
	PARAM_SAVE(aspect_ratio_factor_);
	PARAM_SAVE(near_clip_distance_);
	PARAM_SAVE(far_clip_distance_);
	return param_map;
}

std::pair<std::unique_ptr<Camera>, ParamResult> Camera::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::Angular: return AngularCamera::factory(logger, name, param_map);
		case Type::Perspective: return PerspectiveCamera::factory(logger, name, param_map);
		case Type::Architect: return ArchitectCamera::factory(logger, name, param_map);
		case Type::Orthographic: return OrthographicCamera::factory(logger, name, param_map);
		case Type::Equirectangular: return EquirectangularCamera::factory(logger, name, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

Camera::Camera(Logger &logger, ParamResult &param_result, const ParamMap &param_map) :
		params_{param_result, param_map},
		aspect_ratio_{params_.aspect_ratio_factor_ * static_cast<float>(params_.resy_) / static_cast<float>(params_.resx_)},
		logger_{logger}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());

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

std::string Camera::exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const
{
	std::stringstream ss;
	const auto param_map{getAsParamMap(only_export_non_default_parameters)};
	ss << std::string(indent_level, '\t') << "<camera>" << std::endl;
	ss << param_map.exportMap(indent_level + 1, container_export_type, only_export_non_default_parameters, getParamMetaMap(), {"type"});
	ss << std::string(indent_level, '\t') << "</camera>" << std::endl;
	return ss.str();
}

} //namespace yafaray
