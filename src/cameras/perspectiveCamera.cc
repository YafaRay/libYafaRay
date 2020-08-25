/****************************************************************************
 *
 * 			camera.cc: Camera implementation
 *      This is part of the yafray package
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

#include <cameras/perspectiveCamera.h>
#include <core_api/environment.h>
#include <core_api/params.h>

__BEGIN_YAFRAY

perspectiveCam_t::perspectiveCam_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
                                   int _resx, int _resy, float aspect,
                                   float df, float ap, float dofd, bokehType bt, bkhBiasType bbt, float bro,
                                   float const near_clip_distance, float const far_clip_distance) :
	camera_t(pos, look, up,  _resx, _resy, aspect, near_clip_distance, far_clip_distance), bkhtype(bt), bkhbias(bbt), aperture(ap), focal_distance(df), dof_distance(dofd)
{
	// Initialize camera specific plane coordinates
	setAxis(camX, camY, camZ);

	fdist = (look - pos).length();
	A_pix = aspect_ratio / (df * df);

	int ns = (int)bkhtype;
	if((ns >= 3) && (ns <= 6))
	{
		float w = degToRad(bro), wi = (M_2PI) / (float)ns;
		ns = (ns + 2) * 2;
		LS.resize(ns);
		for(int i = 0; i < ns; i += 2)
		{
			LS[i] = fCos(w);
			LS[i + 1] = fSin(w);
			w += wi;
		}
	}
}

perspectiveCam_t::~perspectiveCam_t()
{
}

void perspectiveCam_t::setAxis(const vector3d_t &vx, const vector3d_t &vy, const vector3d_t &vz)
{
	camX = vx;
	camY = vy;
	camZ = vz;

	dof_rt = aperture * camX; // for dof, premul with aperture
	dof_up = aperture * camY;

	vright = camX;
	vup = aspect_ratio * camY;
	vto = (camZ * focal_distance) - 0.5 * (vup + vright);
	vup /= (float)resy;
	vright /= (float)resx;
}

void perspectiveCam_t::biasDist(float &r) const
{
	switch(bkhbias)
	{
		case BB_CENTER:
			r = fSqrt(fSqrt(r) * r);
			break;
		case BB_EDGE:
			r = fSqrt((float)1.0 - r * r);
			break;
		default:
		case BB_NONE:
			r = fSqrt(r);
	}
}

void perspectiveCam_t::sampleTSD(float r1, float r2, float &u, float &v) const
{
	float fn = (float)bkhtype;
	int idx = int(r1 * fn);
	r1 = (r1 - ((float)idx) / fn) * fn;
	biasDist(r1);
	float b1 = r1 * r2;
	float b0 = r1 - b1;
	idx <<= 1;
	u = LS[idx] * b0 + LS[idx + 2] * b1;
	v = LS[idx + 1] * b0 + LS[idx + 3] * b1;
}

void perspectiveCam_t::getLensUV(float r1, float r2, float &u, float &v) const
{
	switch(bkhtype)
	{
		case BK_TRI:
		case BK_SQR:
		case BK_PENTA:
		case BK_HEXA:
			sampleTSD(r1, r2, u, v);
			break;
		case BK_DISK2:
		case BK_RING:
		{
			float w = (float)M_2PI * r2;
			if(bkhtype == BK_RING) r1 = fSqrt((float)0.707106781 + (float)0.292893218);
			else biasDist(r1);
			u = r1 * fCos(w);
			v = r1 * fSin(w);
			break;
		}
		default:
		case BK_DISK1:
			ShirleyDisk(r1, r2, u, v);
	}
}



ray_t perspectiveCam_t::shootRay(float px, float py, float lu, float lv, float &wt) const
{
	ray_t ray;
	wt = 1;	// for now always 1, except 0 for probe when outside sphere

	ray.from = position;
	ray.dir = vright * px + vup * py + vto;
	ray.dir.normalize();

	ray.tmin = ray_plane_intersection(ray, near_plane);
	ray.tmax = ray_plane_intersection(ray, far_plane);

	if(aperture != 0)
	{
		float u, v;

		getLensUV(lu, lv, u, v);
		vector3d_t LI = dof_rt * u + dof_up * v;
		ray.from += point3d_t(LI);
		ray.dir = (ray.dir * dof_distance) - LI;
		ray.dir.normalize();
	}
	return ray;
}

point3d_t perspectiveCam_t::screenproject(const point3d_t &p) const
{
	point3d_t s;
	vector3d_t dir = p - position;

	// project p to pixel plane:
	float dx = dir * camX;
	float dy = dir * camY;
	float dz = dir * camZ;

	s.x = 2.0f * dx * focal_distance / dz;
	s.y = -2.0f * dy * focal_distance / (dz * aspect_ratio);
	s.z = 0;

	return s;
}

bool perspectiveCam_t::project(const ray_t &wo, float lu, float lv, float &u, float &v, float &pdf) const
{
	// project wo to pixel plane:
	float dx = camX * wo.dir;
	float dy = camY * wo.dir;
	float dz = camZ * wo.dir;
	if(dz <= 0) return false;

	u = dx * focal_distance / dz;
	if(u < -0.5 || u > 0.5) return false;
	u = (u + 0.5) * (float) resx;

	v = dy * focal_distance / (dz * aspect_ratio);
	if(v < -0.5 || v > 0.5) return false;
	v = (v + 0.5) * (float) resy;

	// pdf = 1/A_pix * r^2 / cos(forward, dir), where r^2 is also 1/cos(vto, dir)^2
	float cos_wo = dz; //camZ * wo.dir;
	pdf = 8.f * M_PI / (A_pix *  cos_wo * cos_wo * cos_wo);
	return true;
}

bool perspectiveCam_t::sampleLense() const { return aperture != 0; }

camera_t *perspectiveCam_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	std::string bkhtype = "disk1", bkhbias = "uniform";
	point3d_t from(0, 1, 0), to(0, 0, 0), up(0, 1, 1);
	int resx = 320, resy = 200;
	float aspect = 1, dfocal = 1, apt = 0, dofd = 0, bkhrot = 0;
	float nearClip = 0.0f, farClip = -1.0f;
	std::string viewName = "";

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
	params.getParam("nearClip", nearClip);
	params.getParam("farClip", farClip);
	params.getParam("view_name", viewName);

	bokehType bt = BK_DISK1;
	if(bkhtype == "disk2")			bt = BK_DISK2;
	else if(bkhtype == "triangle")	bt = BK_TRI;
	else if(bkhtype == "square")	bt = BK_SQR;
	else if(bkhtype == "pentagon")	bt = BK_PENTA;
	else if(bkhtype == "hexagon")	bt = BK_HEXA;
	else if(bkhtype == "ring")		bt = BK_RING;
	// bokeh bias
	bkhBiasType bbt = BB_NONE;
	if(bkhbias == "center") 		bbt = BB_CENTER;
	else if(bkhbias == "edge") 		bbt = BB_EDGE;

	perspectiveCam_t *cam = new perspectiveCam_t(from, to, up, resx, resy, aspect, dfocal, apt, dofd, bt, bbt, bkhrot, nearClip, farClip);

	cam->view_name = viewName;

	return cam;
}

__END_YAFRAY
