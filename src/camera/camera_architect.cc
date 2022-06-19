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
#include "common/param.h"

namespace yafaray {

ArchitectCamera::ArchitectCamera(Logger &logger, const Point3 &pos, const Point3 &look, const Point3 &up,
								 int resx, int resy, float aspect,
								 float df, float ap, float dofd, BokehType bt, BkhBiasType bbt, float bro, float const near_clip_distance, float const far_clip_distance)
	: PerspectiveCamera(logger, pos, look, up, resx, resy, aspect, df, ap, dofd, bt, bbt, bro, near_clip_distance, far_clip_distance)
{
	// Initialize camera specific plane coordinates
	setAxis(cam_x_, cam_y_, cam_z_);

	int ns = (int)bkhtype_;
	if((ns >= 3) && (ns <= 6))
	{
		float w = math::degToRad(bro), wi = math::mult_pi_by_2<> / static_cast<float>(ns);
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

void ArchitectCamera::setAxis(const Vec3 &vx, const Vec3 &vy, const Vec3 &vz)
{
	cam_x_ = vx;
	cam_y_ = vy;
	cam_z_ = vz;

	dof_rt_ = cam_x_ * aperture_; // for dof, premul with aperture
	dof_up_ = cam_y_ * aperture_;

	vright_ = cam_x_;
	vup_ = aspect_ratio_ * Vec3{0.f, 0.f, -1.f};
	vto_ = (cam_z_ * focal_distance_) - 0.5 * (vup_ + vright_);
	vup_ /= (float)resy_;
	vright_ /= (float)resx_;
}

Point3 ArchitectCamera::screenproject(const Point3 &p) const
{
	// FIXME
	const Vec3 dir{p - position_};
	// project p to pixel plane:
	const Vec3 camy{0.f, 0.f, 1.f};
	const Vec3 camz{camy ^ cam_x_};
	const Vec3 camx{camz ^camy};

	const float dx = dir * camx;
	const float dy = dir * cam_y_;
	float dz = dir * camz;

	Point3 s;
	s.y() = 2 * dy * focal_distance_ / (dz * aspect_ratio_);
	// Needs focal_distance correction
	const float fod = focal_distance_ * camy * cam_y_ / (camx * cam_x_);
	s.x() = 2 * dx * fod / dz;
	s.z() = 0.f;
	return s;
}

const Camera * ArchitectCamera::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	std::string bkhtype = "disk1", bkhbias = "uniform";
	Point3 from(0, 1, 0), to(0, 0, 0), up(0, 1, 1);
	int resx = 320, resy = 200;
	float aspect = 1, dfocal = 1, apt = 0, dofd = 0, bkhrot = 0;
	float near_clip = 0.0f, far_clip = -1.0f;
	std::string view_name;

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("up", up);
	params.getParam("resx", resx);
	params.getParam("resy", resy);
	params.getParam("focal", dfocal);
	params.getParam("aperture", apt);
	params.getParam("dof_distance", dofd);
	params.getParam("bokeh_type", bkhtype);
	params.getParam("bokeh_bias", bkhbias);
	params.getParam("bokeh_rotation", bkhrot);
	params.getParam("aspect_ratio", aspect);
	params.getParam("nearClip", near_clip);
	params.getParam("farClip", far_clip);
	params.getParam("view_name", view_name);

	BokehType bt = BokehType::BkDisk1;
	if(bkhtype == "disk2")			bt = BokehType::BkDisk2;
	else if(bkhtype == "triangle")	bt = BokehType::BkTri;
	else if(bkhtype == "square")	bt = BokehType::BkSqr;
	else if(bkhtype == "pentagon")	bt = BokehType::BkPenta;
	else if(bkhtype == "hexagon")	bt = BokehType::BkHexa;
	else if(bkhtype == "ring")		bt = BokehType::BkRing;
	// bokeh bias
	BkhBiasType bbt = BkhBiasType::BbNone;
	if(bkhbias == "center") 		bbt = BkhBiasType::BbCenter;
	else if(bkhbias == "edge") 		bbt = BkhBiasType::BbEdge;

	return new ArchitectCamera(logger, from, to, up, resx, resy, aspect, dfocal, apt, dofd, bt, bbt, bkhrot, near_clip, far_clip);
}

} //namespace yafaray
