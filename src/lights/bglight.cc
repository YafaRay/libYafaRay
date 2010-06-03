/****************************************************************************
 *		bglight.cc: a light source using the background
 *      This is part of the yafaray package
 *      Copyright (C) 2006 Mathias Wein (Lynx)
 *      Copyright (C) 2009 Rodrigo Placencia (DarkTide)
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
 
#include <lights/bglight.h>
#include <core_api/background.h>
#include <core_api/texture.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

#define MAX_VSAMPLES 360
#define MAX_USAMPLES 720
#define MIN_SAMPLES 4

#define SMPL_OFF 0.4999f

#define sigma 0.000001f

#define addOff(v) (v + SMPL_OFF)
#define clampSample(s, m) std::max(0, std::min((int)(v), m - 1))

#define multPdf(p0, p1) (p0 * p1)
#define calcPdf(p0, p1, s) std::max( sigma, multPdf(p0, p1) * (float)M_1_2PI * clampZero(sinSample(s)) )
#define calcInvPdf(p0, p1, s) std::max( sigma, (float)M_2PI * sinSample(s) * clampZero(multPdf(p0, p1)) )

inline float clampZero(float val)
{
	if(val > 0.f) return 1.f / val;
	else return 0.f;
}

inline float sinSample(float s)
{
	return fSin(s * M_PI);
}

bgLight_t::bgLight_t(int sampl, bool shootC, bool shootD, bool absIntersect):
light_t(LIGHT_NONE), samples(sampl), shootCaustic(shootC), shootDiffuse(shootD), absInter(absIntersect)
{
	background = NULL;
}

bgLight_t::~bgLight_t()
{
	for(int i = 0; i < vDist->count; i++) delete uDist[i];
	delete[] uDist;
	delete vDist;
}

void bgLight_t::init(scene_t &scene)
{
	float *fu = new float[MAX_USAMPLES];
	float *fv = new float[MAX_VSAMPLES];
	float inu = 0, inv = 0;
	float fx = 0.f, fy = 0.f;
	float sintheta = 0.f;
	int nu = 0, nv = MAX_VSAMPLES;
	
	ray_t ray;
	ray.from = point3d_t(0.f);
	
	inv = 1.f / (float)nv;
	
	uDist = new pdf1D_t*[nv];
	
	for (int y = 0; y < nv; y++)
	{
		fy = ((float)y + 0.5f) * inv;
		
		sintheta = sinSample(fy);
		
		nu = MIN_SAMPLES + (int)(sintheta * (MAX_USAMPLES - MIN_SAMPLES));
		inu = 1.f / (float)nu;
		
		for(int x = 0; x < nu; x++)
		{
			fx = ((float)x + 0.5f) * inu;
			
			invSpheremap(fx, fy, ray.dir);
			
			fu[x] = background->eval(ray).energy() * sintheta;
		}
		
		uDist[y] = new pdf1D_t(fu, nu);
		
		fv[y] = uDist[y]->integral;
	}
	
	vDist = new pdf1D_t(fv, nv);
	
	delete[] fv;
	delete[] fu;

	bound_t w=scene.getSceneBound();
	worldCenter = 0.5 * (w.a + w.g);
	worldRadius = 0.5 * (w.g - w.a).length();
	aPdf = worldRadius * worldRadius;
	iaPdf = 1.f / aPdf;
	worldPIFactor = (M_2PI * aPdf);
}

inline float bgLight_t::CalcFromSample(float s1, float s2, float &u, float &v, bool inv) const
{
	int iv;
	float pdf1 = 0.f, pdf2 = 0.f;
	
	v = vDist->Sample(s2, &pdf2);
	
	iv = clampSample(addOff(v), vDist->count);
	
	u = uDist[iv]->Sample(s1, &pdf1);
	
	u *= uDist[iv]->invCount;
	v *= vDist->invCount;
	
	if(inv)return calcInvPdf(pdf1, pdf2, v);
	
	return calcPdf(pdf1, pdf2, v);
}

inline float bgLight_t::CalcFromDir(const vector3d_t &dir, float &u, float &v, bool inv) const
{
	int iv, iu;
	float pdf1 = 0.f, pdf2 = 0.f;
	
	spheremap(dir, u, v); // Returns u,v pair in [0,1] range

	iv = clampSample(addOff(v * vDist->count), vDist->count);
	iu = clampSample(addOff(u * uDist[iv]->count), uDist[iv]->count);
	
	pdf1 = uDist[iv]->func[iu] * uDist[iv]->invIntegral;
	pdf2 = vDist->func[iv] * vDist->invIntegral;

	if(inv)return calcInvPdf(pdf1, pdf2, v);
	
	return calcPdf(pdf1, pdf2, v);

}

void bgLight_t::sample_dir(float s1, float s2, vector3d_t &dir, float &pdf, bool inv) const
{
	float u = 0.f, v = 0.f;
	
	pdf = CalcFromSample(s1, s2, u, v, inv);
	
	invSpheremap(u, v, dir);
}

// dir points from surface point to background
float bgLight_t::dir_pdf(const vector3d_t dir) const
{
	float u = 0.f, v = 0.f;
	
	return CalcFromDir(dir, u, v);
}

bool bgLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	float u = 0.f, v = 0.f;
	vector3d_t U, V;
	
	wi.tmax = -1.0;
	
	s.pdf = CalcFromSample(s.s1, s.s2, u, v, false);
	
	invSpheremap(u, v, wi.dir);

	s.col = background->eval(wi);

	return true;
}

bool bgLight_t::intersect(const ray_t &ray, float &t, color_t &col, float &ipdf) const
{
	float u = 0.f, v = 0.f;
	ray_t tr = ray;
	vector3d_t absDir = tr.dir;
	
	if(absInter) absDir = -absDir;
	
	ipdf = CalcFromDir(absDir, u, v, true);

	invSpheremap(u, v, tr.dir);

	col = background->eval(tr);

	return true;
}

color_t bgLight_t::totalEnergy() const
{
	color_t energy = background->eval(ray_t(point3d_t(0,0,0), vector3d_t(0.5, 0.5, 0.5))) * worldPIFactor;
	return energy;
}

color_t bgLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	color_t pcol;
	vector3d_t U, V;
	vector3d_t offs;
	float u, v;
	
	sample_dir(s3, s4, ray.dir, ipdf, true);

	pcol = background->eval(ray);
	ray.dir = -ray.dir;
	
	createCS(ray.dir, U, V);
	ShirleyDisk(s1, s2, u, v);

	offs = u*U + v*V;

	ray.from = worldCenter + worldRadius*(offs - ray.dir);
	
	return pcol * aPdf;
}

color_t bgLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	color_t pcol;
	vector3d_t U, V;
	vector3d_t offs;
	float u, v;
	
	sample_dir(s.s1, s.s2, wo, s.dirPdf, true);
	
	pcol = background->eval(ray_t(point3d_t(0,0,0), wo));
	wo = -wo;
	
	createCS(wo, U, V);
	ShirleyDisk(s.s1, s.s2, u, v);
	
	offs = u*U + v*V;

	s.sp->P = worldCenter + worldRadius*offs - worldRadius*wo;
	s.sp->N = s.sp->Ng = wo;
	s.areaPdf = 1.f;
	s.flags = flags;
	
	return pcol;
}

float bgLight_t::illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const
{
	vector3d_t dir = (sp_light.P - sp.P).normalize();
	return dir_pdf(dir);
}

void bgLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	vector3d_t wi = wo;
	wi.normalize();
	cos_wo = wi.z;
	wi = -wi;
	dirPdf = dir_pdf(wi);
	areaPdf = 1.f;
}

light_t* bgLight_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	int samples = 16;
	bool shootD = true;
	bool shootC = true;
	bool absInt = false;
	
	params.getParam("samples", samples);
	params.getParam("shoot_caustics", shootC);
	params.getParam("shoot_diffuse", shootD);
	params.getParam("abs_intersect", absInt);

	bgLight_t *light = new bgLight_t(samples, shootC, shootD, absInt);

	return light;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("bglight", bgLight_t::factory);
	}
}

__END_YAFRAY

