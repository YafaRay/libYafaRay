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
#include "common/environment.h"
#include "common/param.h"

BEGIN_YAFARAY

Camera *Camera::factory(ParamMap &params, RenderEnvironment &render)
{
	std::string type;
	params.getParam("type", type);
	if(type == "angular") return AngularCamera::factory(params, render);
	else if(type == "perspective") return PerspectiveCamera::factory(params, render);
	else if(type == "architect") return ArchitectCamera::factory(params, render);
	else if(type == "orthographic") return OrthographicCamera::factory(params, render);
	else if(type == "equirectangular") return EquirectangularCamera::factory(params, render);
	else return nullptr;
}

Camera::Camera(const Point3 &pos, const Point3 &look, const Point3 &up, int resx, int resy, float aspect, const float near_clip_distance, const float far_clip_distance) :
		position_(pos), resx_(resx), resy_(resy), aspect_ratio_(aspect * (float)resy_ / (float)resx_), camera_name_(""), view_name_("")
{
	// Calculate and store camera axis
	cam_y_ = up - position_;
	cam_z_ = look - position_;
	cam_x_ = cam_z_ ^ cam_y_;
	cam_y_ = cam_z_ ^ cam_x_;
	cam_x_.normalize();
	cam_y_.normalize();
	cam_z_.normalize();

	near_plane_.n_ = cam_z_;
	near_plane_.p_ = Vec3(position_) + cam_z_ * near_clip_distance;

	far_plane_.n_ = cam_z_;
	far_plane_.p_ = Vec3(position_) + cam_z_ * far_clip_distance;

	near_clip_ = near_clip_distance;
	far_clip_ = far_clip_distance;
}

END_YAFARAY
