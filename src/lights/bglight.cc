/****************************************************************************
 * 			bglight.cc: a light source using the background
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
	else return 1.f;
}

inline float sinSample(float s)
{
	float ang = s * M_PI;
	return fSin(ang);
}

bgLight_t::bgLight_t(background_t *bg, int sampl): samples(sampl), background(bg)
{
	initIS();
}

bgLight_t::~bgLight_t()
{
	for(int i = 0; i < vDist->count; i++) delete uDist[i];
	delete[] uDist;
	delete vDist;
}

void bgLight_t::initIS()
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
}


float bgLight_t::CalcFromSample(float s1, float s2, float &u, float &v, bool inv) const
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

float bgLight_t::CalcFromDir(const vector3d_t &dir, float &u, float &v, bool inv) const
{
	int iv, iu;
	float invInt;
	float pdf1 = 0.f, pdf2 = 0.f;
	
	spheremap(dir, u, v);

	u = u * 0.5f + 0.5f;
	v = v * 0.5f + 0.5f;

	iv = clampSample(addOff(v * vDist->count), vDist->count);
	iu = clampSample(addOff(u * uDist[iv]->count), uDist[iv]->count);
	
	invInt = uDist[iv]->invIntegral * vDist->invIntegral;
	
	pdf1 = uDist[iv]->func[iu] * invInt;
	pdf2 = vDist->func[iv] * invInt;

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
	
	wi.tmax = -1.0;
	
	s.pdf = CalcFromSample(s.s1, s.s2, u, v);
	
	invSpheremap(u, v, wi.dir);
	
	s.col = background->eval(wi);
	
	return true;
}

bool bgLight_t::intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const
{
	float u = 0.f, v = 0.f;
	ray_t tr = ray;
	
	ipdf = CalcFromDir(tr.dir, u, v, true);
	
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
	float ipdf2, u, v;
	
	sample_dir(s3, s4, ray.dir, ipdf2, true);

	pcol = background->eval(ray);
	ray.dir = -ray.dir;
	
	createCS(ray.dir, U, V);
	ShirleyDisk(s1, s2, u, v);

	offs = u*U + v*V;

	ray.from = worldCenter + worldRadius*offs - worldRadius*ray.dir;
	
	ipdf = ipdf2 * worldPIFactor;
	
	return pcol;
}

color_t bgLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	color_t pcol;
	vector3d_t U, V;
	vector3d_t offs;
	float u, v;
	
	sample_dir(s.s3, s.s4, wo, s.dirPdf, false);
	
	pcol = background->eval(ray_t(point3d_t(0,0,0), wo));
	wo = -wo;
	
	createCS(wo, U, V);
	ShirleyDisk(s.s1, s.s2, u, v);
	
	offs = u*U + v*V;

	s.sp->P = worldCenter + worldRadius*offs - worldRadius*wo;
	s.sp->N = s.sp->Ng = wo;
	s.areaPdf = iaPdf;
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
	cos_wo = 1.f;
	vector3d_t wi = -wo;
	dirPdf = dir_pdf(wi);
	areaPdf = iaPdf;
}

void bgLight_t::init(scene_t &scene)
{
	bound_t w=scene.getSceneBound();
	worldCenter = 0.5 * (w.a + w.g);
	worldRadius = 0.5 * (w.g - w.a).length();
	std::cout << "bgLight: World Radius: " << worldRadius << "\n";
	aPdf = worldRadius * worldRadius;
	iaPdf = 1.f / aPdf;
	worldPIFactor = (M_2PI * aPdf);
}

__END_YAFRAY

