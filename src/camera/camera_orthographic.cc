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
#include "common/param.h"

BEGIN_YAFARAY

OrthographicCamera::OrthographicCamera(const Point3 &pos, const Point3 &look, const Point3 &up,
									   int resx, int resy, float aspect, float scale, float const near_clip_distance, float const far_clip_distance)
	: Camera(pos, look, up, resx, resy, aspect, near_clip_distance, far_clip_distance), scale_(scale)
{
	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);
}

void OrthographicCamera::setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	vright_ = cam_x_;
	vup_ = aspect_ratio_ * cam_y_;
	vto_ = cam_z_;
	pos_ = position_ - 0.5 * scale_ * (vup_ + vright_);
	vup_     *= scale_ / (float)resy_;
	vright_  *= scale_ / (float)resx_;
}


Ray OrthographicCamera::shootRay(float px, float py, float lu, float lv, float &wt) const
{
	Ray ray;
	wt = 1;	// for now always 1, except 0 for probe when outside sphere
	ray.from_ = pos_ + vright_ * px + vup_ * py;
	ray.dir_ = vto_;

	ray.tmin_ = rayPlaneIntersection_global(ray, near_plane_);
	ray.tmax_ = rayPlaneIntersection_global(ray, far_plane_);

	return ray;
}

Point3 OrthographicCamera::screenproject(const Point3 &p) const
{
	Point3 s;
	Vec3 dir = p - pos_;
	// Project p to pixel plane

	float dz = cam_z_ * dir;

	Vec3 proj = dir - dz * cam_z_;

	s.x_ = 2 * (proj * cam_x_ / scale_) - 1.0f;
	s.y_ = - 2 * proj * cam_y_ / (aspect_ratio_ * scale_) + 1.0f;
	s.z_ = 0;

	return s;
}

std::unique_ptr<Camera> OrthographicCamera::factory(ParamMap &params, const Scene &scene)
{
	Point3 from(0, 1, 0), to(0, 0, 0), up(0, 1, 1);
	int resx = 320, resy = 200;
	double aspect = 1.0, scale = 1.0;
	float near_clip = 0.0f, far_clip = -1.0f;
	std::string view_name = "";

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("up", up);
	params.getParam("resx", resx);
	params.getParam("resy", resy);
	params.getParam("scale", scale);
	params.getParam("aspect_ratio", aspect);
	params.getParam("nearClip", near_clip);
	params.getParam("farClip", far_clip);
	params.getParam("view_name", view_name);

	return std::unique_ptr<Camera>(new OrthographicCamera(from, to, up, resx, resy, aspect, scale, near_clip, far_clip));
}

END_YAFARAY
