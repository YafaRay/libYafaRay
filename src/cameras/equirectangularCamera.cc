/****************************************************************************
 *
 *      equirectangularCamera.cc: Camera implementation
 *      This is part of the yafray package
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

#include <cameras/equirectangularCamera.h>
#include <core_api/environment.h>

__BEGIN_YAFRAY

equirectangularCam_t::equirectangularCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
                           int _resx, int _resy, float asp,
                           float const near_clip_distance, float const far_clip_distance) :
    camera_t(pos, look, up, _resx, _resy, asp, near_clip_distance, far_clip_distance)
{
	// Initialize camera specific plane coordinates
	setAxis(camX,camY,camZ);
}

void equirectangularCam_t::setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz)
{
	camX = vx;
	camY = vy;
	camZ = vz;

	vright = camX;
	vup = camY;
	vto = camZ;
}

ray_t equirectangularCam_t::shootRay(float px, float py, float lu, float lv, float &wt) const
{
	ray_t ray;

	wt = 1;	// for now always 1, or 0 when circular and outside angle
	ray.from = position;
	float u = 2.f * px/(float)resx - 1.f;
	float v = 2.f * py/(float)resy - 1.f;
	const float phi = M_PI * u;
	const float theta = M_PI_2 * v;
	ray.dir = fCos(theta)*(fCos(phi)*vto + fSin(phi)*vright ) + fSin(theta)*vup;

    ray.tmin = ray_plane_intersection(ray, near_plane);
    ray.tmax = ray_plane_intersection(ray, far_plane);

	return ray;
}

camera_t* equirectangularCam_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	point3d_t from(0,1,0), to(0,0,0), up(0,1,1);
	int resx=320, resy=200;
	double aspect=1.0;
	float nearClip = 0.0f, farClip = -1.0e38f;
	std::string viewName = "";

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("up", up);
	params.getParam("resx", resx);
	params.getParam("resy", resy);
	params.getParam("aspect_ratio", aspect);
    params.getParam("nearClip", nearClip);
    params.getParam("farClip", farClip);
    params.getParam("view_name", viewName);

    equirectangularCam_t *cam = new equirectangularCam_t(from, to, up, resx, resy, aspect, nearClip, farClip);

	cam->view_name = viewName;

	return cam;
}

point3d_t equirectangularCam_t::screenproject(const point3d_t &p) const
{
	//FIXME
	point3d_t s;
	vector3d_t dir = vector3d_t(p) - vector3d_t(position);
	dir.normalize();
	
	// project p to pixel plane:	
	float dx = camX * dir;
	float dy = camY * dir;
	float dz = camZ * dir;

	s.x = -dx / (4.0 * M_PI * dz);
	s.y = -dy / (4.0 * M_PI * dz);
	s.z = 0;
	
	return s;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("equirectangular",	equirectangularCam_t::factory);
	}

}

__END_YAFRAY
