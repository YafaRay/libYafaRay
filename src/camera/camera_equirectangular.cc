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
#include "common/environment.h"
#include "common/param.h"

BEGIN_YAFARAY

EquirectangularCamera::EquirectangularCamera(const Point3 &pos, const Point3 &look, const Point3 &up,
											 int resx, int resy, float asp,
											 float const near_clip_distance, float const far_clip_distance) :
		Camera(pos, look, up, resx, resy, asp, near_clip_distance, far_clip_distance)
{
	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);
}

void EquirectangularCamera::setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	vright_ = cam_x_;
	vup_ = cam_y_;
	vto_ = cam_z_;
}

Ray EquirectangularCamera::shootRay(float px, float py, float lu, float lv, float &wt) const
{
	Ray ray;

	wt = 1;	// for now always 1, or 0 when circular and outside angle
	ray.from_ = position_;
	float u = 2.f * px / (float)resx_ - 1.f;
	float v = 2.f * py / (float)resy_ - 1.f;
	const float phi = M_PI * u;
	const float theta = M_PI_2 * v;
	ray.dir_ = fCos__(theta) * (fCos__(phi) * vto_ + fSin__(phi) * vright_) + fSin__(theta) * vup_;

	ray.tmin_ = rayPlaneIntersection__(ray, near_plane_);
	ray.tmax_ = rayPlaneIntersection__(ray, far_plane_);

	return ray;
}

Camera *EquirectangularCamera::factory(ParamMap &params, RenderEnvironment &render)
{
	Point3 from(0, 1, 0), to(0, 0, 0), up(0, 1, 1);
	int resx = 320, resy = 200;
	double aspect = 1.0;
	float near_clip = 0.0f, far_clip = -1.0e38f;
	std::string view_name = "";

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("up", up);
	params.getParam("resx", resx);
	params.getParam("resy", resy);
	params.getParam("aspect_ratio", aspect);
	params.getParam("nearClip", near_clip);
	params.getParam("farClip", far_clip);
	params.getParam("view_name", view_name);

	EquirectangularCamera *cam = new EquirectangularCamera(from, to, up, resx, resy, aspect, near_clip, far_clip);

	cam->view_name_ = view_name;

	return cam;
}

Point3 EquirectangularCamera::screenproject(const Point3 &p) const
{
	//FIXME
	Point3 s;
	Vec3 dir = Vec3(p) - Vec3(position_);
	dir.normalize();

	// project p to pixel plane:
	float dx = cam_x_ * dir;
	float dy = cam_y_ * dir;
	float dz = cam_z_ * dir;

	s.x_ = -dx / (4.0 * M_PI * dz);
	s.y_ = -dy / (4.0 * M_PI * dz);
	s.z_ = 0;

	return s;
}

END_YAFARAY
