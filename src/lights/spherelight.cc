/****************************************************************************
 * 			spherelight.cc: a spherical area light source
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

#include <core_api/light.h>
#include <core_api/surface.h>
#include <core_api/environment.h>
#include <core_api/object3d.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

/*! sphere lights are *drumroll* lights with a sphere shape.
	They only emit light on the outside! On the inside it is somewhat pointless,
	because in that case, sampling from BSDF would _always_ be at least as efficient,
	and in most cases much smarter, so use just geometry with emiting material...
	The light samples from the cone in which the light is visible, instead of directly
	from the sphere surface (thanks to PBRT for the hint)
*/

class sphereLight_t : public light_t
{
	public:
		sphereLight_t(const point3d_t &c, PFLOAT rad, const color_t &col, CFLOAT inte, int nsam);
		~sphereLight_t();
		virtual void init(scene_t &scene);
		virtual color_t totalEnergy() const;
		virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
		virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const;
		virtual bool diracLight() const { return false; }
		virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
		virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi)const { return false; }
		virtual bool canIntersect() const{ return true; }
		virtual bool intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const;
		virtual float illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const;
		virtual void emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const;
		virtual int nSamples() const { return samples; }
		static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		point3d_t center;
		PFLOAT radius, square_radius, square_radius_epsilon;
		color_t color; //!< includes intensity amplification! so...
		int samples;
		unsigned int objID;
		float area, invArea;
};

sphereLight_t::sphereLight_t(const point3d_t &c, PFLOAT rad, const color_t &col, CFLOAT inte, int nsam):
	center(c), radius(rad), samples(nsam)
{
	color = col*inte;
	square_radius = radius*radius;
	square_radius_epsilon = square_radius * 1.000003815; // ~0.2% larger radius squared
	area = square_radius * 4.0 * M_PI;
	invArea = 1.f/area;
}

sphereLight_t::~sphereLight_t(){ }

void sphereLight_t::init(scene_t &scene)
{
	if(objID)
	{
		object3d_t *obj = scene.getObject(objID);
		if(obj) obj->setLight(this);
		else std::cout << "areaLight_t::init(): invalid object ID given!\n";
	}
}

color_t sphereLight_t::totalEnergy() const { return color * area /* * M_PI */; }

inline bool sphereIntersect(const ray_t &ray, const point3d_t &c, PFLOAT R2, PFLOAT &d1, PFLOAT &d2)
{
	vector3d_t vf=ray.from-c;
	PFLOAT ea=ray.dir*ray.dir;
	PFLOAT eb=2.0*vf*ray.dir;
	PFLOAT ec=vf*vf-R2;
	PFLOAT osc=eb*eb-4.0*ea*ec;
	if(osc<0){ d1 = fSqrt(ec/ea); return false; } // assume tangential hit/miss condition => Pythagoras
	osc=fSqrt(osc);
	d1=(-eb-osc)/(2.0*ea);
	d2=(-eb+osc)/(2.0*ea);
	return true;
}

bool sphereLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	vector3d_t cdir = center - sp.P;
	PFLOAT dist_sqr = cdir.lengthSqr();
	if(dist_sqr <= square_radius) return false; //only emit light on the outside!
	
	PFLOAT dist = fSqrt(dist_sqr);
	PFLOAT idist_sqr = 1.f/(dist_sqr);
	PFLOAT cosAlpha = fSqrt(1.f - square_radius * idist_sqr);
	cdir *= 1.f/dist;
	vector3d_t du, dv;
	createCS(cdir, du, dv);
	
	wi.dir = sampleCone(cdir, du, dv, cosAlpha, s.s1, s.s2);
	PFLOAT d1, d2;
	if( !sphereIntersect(wi, center, square_radius_epsilon, d1, d2) )
	{
		return false;
	}
	wi.tmax = d1;
	
	// pdf = 1.f / (2.f * M_PI * (1.f - cos(cone_angle)));
	s.pdf = 1.f / (2.f * (1.f - cosAlpha));
	s.col = color;
	s.flags = flags;
	if(s.sp)
	{
		s.sp->P = wi.from + d1 * wi.dir;
		s.sp->N = s.sp->Ng = (s.sp->P - center).normalize();
	}
	return true;
}

bool sphereLight_t::intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const
{
	PFLOAT d1, d2;
	if( sphereIntersect(ray, center, square_radius, d1, d2) )
	{
		vector3d_t cdir = center - ray.from;
		PFLOAT dist_sqr = cdir.lengthSqr();
		if(dist_sqr <= square_radius) return false; //only emit light on the outside!
		// PFLOAT dist = sqrt(dist_sqr);
		PFLOAT idist_sqr = 1.f/(dist_sqr);
		PFLOAT cosAlpha = fSqrt(1.f - square_radius * idist_sqr);
		ipdf = 2.f * (1.f - cosAlpha);
		return true;
	}
	return false;
}

float sphereLight_t::illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const
{
	vector3d_t cdir = center - sp.P;
	PFLOAT dist_sqr = cdir.lengthSqr();
	if(dist_sqr <= square_radius) return 0.f; //only emit light on the outside!
	PFLOAT idist_sqr = 1.f/(dist_sqr);
	PFLOAT cosAlpha = fSqrt(1.f - square_radius * idist_sqr);
	return 1.f / (2.f * (1.f - cosAlpha));
}

void sphereLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	areaPdf = invArea * M_PI;
	cos_wo = wo*sp.N;
	//! unfinished! use real normal, sp.N might be approximation by mesh...
	dirPdf = cos_wo > 0 ? cos_wo : 0.f;
}

color_t sphereLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	vector3d_t sdir = SampleSphere(s3, s4);
	ray.from = center + radius*sdir;
	vector3d_t du, dv;
	createCS(sdir, du, dv);
	ray.dir = SampleCosHemisphere(sdir, du, dv, s1, s2);
	ipdf = area;
	return color;
}

color_t sphereLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	vector3d_t sdir = SampleSphere(s.s3, s.s4);
	s.sp->P = center + radius*sdir;
	s.sp->N = s.sp->Ng = sdir;
	vector3d_t du, dv;
	createCS(sdir, du, dv);
	wo = SampleCosHemisphere(sdir, du, dv, s.s1, s.s2);
	s.dirPdf = std::fabs(sdir * wo);
	s.areaPdf = invArea * M_PI;
	s.flags = flags;
	return color;
}

light_t *sphereLight_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	point3d_t from(0.0);
	color_t color(1.0);
	CFLOAT power = 1.0;
	float radius = 1.f;
	int samples = 4;
	int object = 0;

	params.getParam("from",from);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("radius",radius);
	params.getParam("samples",samples);
	params.getParam("object", object);
	
	sphereLight_t *light = new sphereLight_t(from, radius, color, power, samples);
	light->objID = (unsigned int)object;
	return light;
}

extern "C"
{	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("spherelight", sphereLight_t::factory);
	}
}

__END_YAFRAY
