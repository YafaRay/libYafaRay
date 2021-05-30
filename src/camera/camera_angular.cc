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

#include "camera/camera_angular.h"
#include "common/param.h"

BEGIN_YAFARAY

AngularCamera::AngularCamera(Logger &logger, const Point3 &pos, const Point3 &look, const Point3 &up,
							 int resx, int resy, float asp, float angle, float max_angle, bool circ, const Projection &projection,
							 float const near_clip_distance, float const far_clip_distance) :
		Camera(logger, pos, look, up, resx, resy, asp, near_clip_distance, far_clip_distance), max_radius_(max_angle / angle), circular_(circ), projection_(projection)
{
	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);

	if(projection == Projection::Orthographic) focal_length_ = 1.f / math::sin(angle);
	else if(projection == Projection::Stereographic) focal_length_ = 1.f / 2.f / std::tan(angle / 2.f);
	else if(projection == Projection::EquisolidAngle) focal_length_ = 1.f / 2.f / math::sin(angle / 2.f);
	else if(projection == Projection::Rectilinear) focal_length_ = 1.f / std::tan(angle);
	else focal_length_ = 1.f / angle; //By default, AngularProjection::Equidistant
}

void AngularCamera::setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	vright_ = cam_x_;
	vup_ = cam_y_;
	vto_ = cam_z_;
}

Ray AngularCamera::shootRay(float px, float py, float lu, float lv, float &wt) const
{
	wt = 1.f; // for now always 1, or 0 when circular and outside angle
	Ray ray;
	ray.from_ = position_;
	const float u = 1.f - 2.f * (px / (float)resx_);
	float v = 2.f * (py / (float)resy_) - 1.f;
	v *= aspect_ratio_;
	const float radius = math::sqrt(u * u + v * v);
	if(circular_ && radius > max_radius_) { wt = 0; return ray; }
	float theta = 0;
	if(!((u == 0) && (v == 0))) theta = atan2(v, u);
	float phi = 0.f;
	if(projection_ == Projection::Orthographic) phi = math::asin(radius / focal_length_);
	else if(projection_ == Projection::Stereographic) phi = 2.f * std::atan(radius / (2.f * focal_length_));
	else if(projection_ == Projection::EquisolidAngle) phi = 2.f * math::asin(radius / (2.f * focal_length_));
	else if(projection_ == Projection::Rectilinear) phi = std::atan(radius / focal_length_);
	else phi = radius / focal_length_; //By default, AngularProjection::Equidistant
	//float sp = sin(phi);
	ray.dir_ = math::sin(phi) * (math::cos(theta) * vright_ + math::sin(theta) * vup_) + math::cos(phi) * vto_;
	ray.tmin_ = near_plane_.rayIntersection(ray);
	ray.tmax_ = far_plane_.rayIntersection(ray);
	return ray;
}

std::unique_ptr<Camera> AngularCamera::factory(Logger &logger, ParamMap &params, const Scene &scene)
{
	Point3 from(0, 1, 0), to(0, 0, 0), up(0, 1, 1);
	int resx = 320, resy = 200;
	double aspect = 1.0, angle_degrees = 90, max_angle_degrees = 90;
	Projection projection = Projection::Equidistant;
	std::string projection_string;
	bool circular = true, mirrored = false;
	float near_clip = 0.0f, far_clip = -1.0e38f;
	std::string view_name;

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("up", up);
	params.getParam("resx", resx);
	params.getParam("resy", resy);
	params.getParam("aspect_ratio", aspect);
	params.getParam("angle", angle_degrees);
	max_angle_degrees = angle_degrees;
	params.getParam("max_angle", max_angle_degrees);
	params.getParam("circular", circular);
	params.getParam("mirrored", mirrored);
	params.getParam("projection", projection_string);
	params.getParam("nearClip", near_clip);
	params.getParam("farClip", far_clip);
	params.getParam("view_name", view_name);

	if(projection_string == "orthographic") projection = Projection::Orthographic;
	else if(projection_string == "stereographic") projection = Projection::Stereographic;
	else if(projection_string == "equisolid_angle") projection = Projection::EquisolidAngle;
	else if(projection_string == "rectilinear") projection = Projection::Rectilinear;
	else projection = Projection::Equidistant;

	auto cam = std::unique_ptr<AngularCamera>(new AngularCamera(logger, from, to, up, resx, resy, aspect, angle_degrees * M_PI / 180.f, max_angle_degrees * M_PI / 180.f, circular, projection, near_clip, far_clip));
	if(mirrored) cam->vright_ *= -1.0;
	return cam;
}

Point3 AngularCamera::screenproject(const Point3 &p) const
{
	//FIXME
	Vec3 dir = p - position_;
	dir.normalize();
	// project p to pixel plane:
	const float dx = cam_x_ * dir;
	const float dy = cam_y_ * dir;
	const float dz = cam_z_ * dir;
	return { -dx / (4.f * static_cast<float>(M_PI) * dz), -dy / (4.f * static_cast<float>(M_PI) * dz), 0.f };
}

END_YAFARAY
