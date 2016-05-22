/****************************************************************************
 * 			pointlight.cc: a simple point light source
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
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

class pointLight_t : public light_t
{
  public:
	pointLight_t(const point3d_t &pos, const color_t &col, CFLOAT inte, bool bLightEnabled=true, bool bCastShadows=true);
	virtual color_t totalEnergy() const { return color * 4.0f * M_PI; }
	virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
	virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const;
	virtual bool diracLight() const { return true; }
	virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
	virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const;
	virtual void emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const;
	static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
  protected:
	point3d_t position;
	color_t color;
	float intensity;
};

pointLight_t::pointLight_t(const point3d_t &pos, const color_t &col, CFLOAT inte, bool bLightEnabled, bool bCastShadows):
	light_t(LIGHT_SINGULAR), position(pos)
{
	lLightEnabled = bLightEnabled;
    lCastShadows = bCastShadows;
    color = col * inte;
	intensity = color.energy();
}

bool pointLight_t::illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const
{	
	if( photonOnly() ) return false;
	
	vector3d_t ldir(position - sp.P);
	PFLOAT dist_sqr = ldir.x*ldir.x + ldir.y*ldir.y + ldir.z*ldir.z;
	PFLOAT dist = fSqrt(dist_sqr);
	PFLOAT idist_sqr = 0.0;
	if(dist == 0.0) return false;
	
	idist_sqr = 1.f/(dist_sqr);
	ldir *= 1.f/dist;
	
	wi.tmax = dist;
	wi.dir = ldir;
	
	col = color * (CFLOAT)idist_sqr;
	return true;
}

bool pointLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	if( photonOnly() ) return false;
	
	// bleh...
	vector3d_t ldir(position - sp.P);
	PFLOAT dist_sqr = ldir.x*ldir.x + ldir.y*ldir.y + ldir.z*ldir.z;
	PFLOAT dist = fSqrt(dist_sqr);
	if(dist == 0.0) return false;
	
	ldir *= 1.f/dist;
	
	wi.tmax = dist;
	wi.dir = ldir;
	
	s.flags = flags;
	s.col =  color;
	s.pdf = dist_sqr;
	return true;
}

color_t pointLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	ray.from = position;
	ray.dir = SampleSphere(s1, s2);
	ipdf = 4.0f * M_PI;
	return color;
}

color_t pointLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	s.sp->P = position;
	wo = SampleSphere(s.s1, s.s2);
	s.flags = flags;
	s.dirPdf = 0.25f;
	s.areaPdf = 1.f;
	return color;
}

void pointLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	areaPdf = 1.f;
	dirPdf = 0.25f;
	cos_wo = 1.f;
}

light_t *pointLight_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	point3d_t from(0.0);
	color_t color(1.0);
	CFLOAT power = 1.0;
	bool lightEnabled = true;
	bool castShadows = true;
	bool shootD = true;
	bool shootC = true;
	bool pOnly = false;

	params.getParam("from",from);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("light_enabled", lightEnabled);
	params.getParam("cast_shadows", castShadows);
	params.getParam("shoot_caustics", shootC);
	params.getParam("shoot_diffuse", shootD);
	params.getParam("photon_only",pOnly);
	

	pointLight_t *light = new pointLight_t(from, color, power, lightEnabled, castShadows);

	light->lShootCaustic = shootC;
	light->lShootDiffuse = shootD;
	light->lPhotonOnly = pOnly;

	return light;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("pointlight",pointLight_t::factory);
	}
}

__END_YAFRAY


