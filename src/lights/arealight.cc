/****************************************************************************
 * 			arealight.cc: a rectangular area light source
 *      This is part of the yafray package
 *      Copyright (C) 2006  Mathias Wein
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
 */

#include <core_api/surface.h>
#include <core_api/object3d.h>
#include <lights/arealight.h>
#include <lights/meshlight.h>
#include <lights/bgportallight.h>
#include <utilities/sample_utils.h>
#include <iostream>

__BEGIN_YAFRAY

//int hit_t1=0, hit_t2=0;

areaLight_t::areaLight_t(const point3d_t &c, const vector3d_t &v1, const vector3d_t &v2,
				const color_t &col,CFLOAT inte, int nsam, bool bLightEnabled, bool bCastShadows):
				corner(c), toX(v1), toY(v2), samples(nsam), intensity(inte), lLightEnabled(bLightEnabled), lCastShadows(bCastShadows)
{
	fnormal = toY^toX; //f normal is "flipped" normal direction...
	color = col*inte * M_PI;
	area = fnormal.normLen();
	invArea = 1.0/area;
	
	normal = -fnormal;
	du = toX;
	du.normalize();
	dv = normal^du;
	c2 = corner + toX;
	c3 = corner + (toX + toY);
	c4 = corner + toY;
}

areaLight_t::~areaLight_t()
{
	// Empty
}

void areaLight_t::init(scene_t &scene)
{
	if(objID)
	{
		object3d_t *obj = scene.getObject(objID);
		if(obj) obj->setLight(this);
		else Y_INFO << "AreaLight: Invalid object ID given!" << yendl;
	}
}

color_t areaLight_t::totalEnergy() const { return color * area; }

bool areaLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	//get point on area light and vector to surface point:
	point3d_t p = corner + s.s1*toX + s.s2*toY;
	vector3d_t ldir = p - sp.P;
	//normalize vec and compute inverse square distance
	PFLOAT dist_sqr = ldir.lengthSqr();
	PFLOAT dist = fSqrt(dist_sqr);
	if(dist <= 0.0) return false;
	ldir *= 1.f/dist;
	PFLOAT cos_angle = ldir*fnormal;
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0) return false;

	// fill direction
	wi.tmax = dist;
	wi.dir = ldir;
	
	s.col = color;
	// pdf = distance^2 / area * cos(norm, ldir);
	s.pdf = dist_sqr*M_PI / (area * cos_angle);
	s.flags = LIGHT_NONE; // no delta functions...
	if(s.sp)
	{
		s.sp->P = p;
		s.sp->N = s.sp->Ng = normal;
	}
	return true;
}

color_t areaLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	ipdf = area/*  * M_PI */; // really two pi?
	ray.from = corner + s3*toX + s4*toY;
	ray.dir = SampleCosHemisphere(normal, du, dv, s1, s2);
	return color;
}

color_t areaLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	s.areaPdf = invArea * M_PI;
	s.sp->P = corner + s.s3*toX + s.s4*toY;
	wo = SampleCosHemisphere(normal, du, dv, s.s1, s.s2);
	s.sp->N = s.sp->Ng = normal;
	s.dirPdf = std::fabs(normal * wo);
	s.flags = LIGHT_NONE; // no delta functions...
	return color; // still not 100% sure this is correct without cosine...
}

inline bool triIntersect(const point3d_t &a, const point3d_t &b, const point3d_t &c, const ray_t &ray, PFLOAT &t)
{
	vector3d_t edge1, edge2, tvec, pvec, qvec;
	PFLOAT det, inv_det, u, v;
	edge1 = b - a;
	edge2 = c - a;
	pvec = ray.dir ^ edge2;
	det = edge1 * pvec;
	if (det == 0.0) return false;
	inv_det = 1.0 / det;
	tvec = ray.from - a;
	u = (tvec*pvec) * inv_det;
	if (u < 0.0 || u > 1.0) return false;
	qvec = tvec^edge1;
	v = (ray.dir*qvec) * inv_det;
	if ((v<0.0) || ((u+v)>1.0) ) return false;
	t = edge2 * qvec * inv_det;

	return true;
}

bool areaLight_t::intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const
{
	PFLOAT cos_angle = ray.dir*fnormal;
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0) return false;
	
	if(!triIntersect(corner, c2, c3, ray, t))
	{
		if(!triIntersect(corner, c3, c4, ray, t)) return false;
	}
	if( !(t > 1.0e-10f) ) return false;
	
	col = color;
	// pdf = distance^2 / area * cos(norm, ldir); ipdf = 1/pdf;
	ipdf = 1.f/(t*t) * area * cos_angle * M_1_PI;
	return true;
}

float areaLight_t::illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const
{
	vector3d_t wi = sp_light.P - sp.P;
	PFLOAT r2 = wi.normLenSqr();
	float cos_n = wi*fnormal;
	return cos_n > 0 ? r2 * M_PI / (area * cos_n) : 0.f;
}

void areaLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	areaPdf = invArea * M_PI;
	cos_wo = wo*sp.N;
	dirPdf = cos_wo > 0 ? cos_wo : 0.f;
}

light_t* areaLight_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	point3d_t corner(0.0);
	point3d_t p1(0.0);
	point3d_t p2(0.0);
	color_t color(1.0);
	CFLOAT power = 1.0;
	int samples = 4;
	int object = 0;
	bool lightEnabled = true;
	bool castShadows = true;

	params.getParam("corner",corner);
	params.getParam("point1",p1);
	params.getParam("point2",p2);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("samples",samples);
	params.getParam("object", object);
	params.getParam("light_enabled", lightEnabled);
	params.getParam("cast_shadows", castShadows);

	areaLight_t *light = new areaLight_t(corner, p1-corner, p2-corner, color, power, samples, lightEnabled, castShadows);
	light->objID = (unsigned int)object;
	return light;
}

extern "C"
{
	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("arealight", areaLight_t::factory);
		render.registerFactory("bgPortalLight",bgPortalLight_t::factory);
		render.registerFactory("meshlight", meshLight_t::factory);
	}

}
__END_YAFRAY
