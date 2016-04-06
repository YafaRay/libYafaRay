/****************************************************************************
 * 			directional.cc: a directional light, with optinal limited radius
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

class directionalLight_t : public light_t
{
  public:
	directionalLight_t(const point3d_t &pos, vector3d_t dir, const color_t &col, CFLOAT inte, bool inf, float rad, bool bLightEnabled=true, bool bCastShadows=true);
	virtual void init(scene_t &scene);
	virtual color_t totalEnergy() const { return color * radius*radius * M_PI; }
	virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
	virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const;
	virtual bool diracLight() const { return true; }
	virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
	virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const;
	static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
  protected:
	point3d_t position;
	color_t color;
	vector3d_t direction, du, dv;
	float intensity;
	PFLOAT radius;
	float areaPdf;
	PFLOAT worldRadius;
	bool infinite;
	int majorAxis; //!< the largest component of direction
};

directionalLight_t::directionalLight_t(const point3d_t &pos, vector3d_t dir, const color_t &col, CFLOAT inte, bool inf, float rad, bool bLightEnabled, bool bCastShadows):
	light_t(LIGHT_DIRACDIR), position(pos), direction(dir), radius(rad), infinite(inf)
{
    lLightEnabled = bLightEnabled;
    lCastShadows = bCastShadows;
	color = col * inte;
	intensity = color.energy();
	direction.normalize();
	createCS(direction, du, dv);
	vector3d_t &d = direction;
	majorAxis = (d.x>d.y) ? ((d.x>d.z) ? 0 : 2) : ((d.y>d.z) ? 1:2 );
}

void directionalLight_t::init(scene_t &scene)
{
	// calculate necessary parameters for photon mapping if the light
	//  is set to illuminate the whole scene:
	bound_t w=scene.getSceneBound();
	worldRadius = 0.5 * (w.g - w.a).length();
	if(infinite)
	{
		position = 0.5 * (w.a + w.g);
		radius = worldRadius;
	}
	areaPdf = 1.f/ (radius*radius); // Pi cancels out with our weird conventions :p
	Y_INFO << "DirectionalLight: pos " << position << " world radius: " << worldRadius << yendl;
}


bool directionalLight_t::illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const
{
	// check if the point is outside of the illuminated cylinder (non-infinite lights)
	if(!infinite)
	{
		vector3d_t vec = position - sp.P;
		PFLOAT dist = (direction ^ vec).length();
		if(dist>radius) return false;
		wi.tmax = (vec*direction);
		if(wi.tmax <= 0.0) return false;
	}
	else
	{
		wi.tmax = -1.0;
	}
	wi.dir = direction;
	
	col = color;
	return true;
}

bool directionalLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	s.pdf = 1.0;
	return illuminate(sp, s.col, wi);
}

color_t directionalLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	//todo
	ray.dir = -direction;
	PFLOAT u, v;
	ShirleyDisk(s1, s2, u, v);
	ray.from = position + radius * (u*du + v*dv);
	if(infinite) ray.from += direction*worldRadius;
	ipdf = M_PI * radius*radius; //4.0f * M_PI;
	return color;
}

color_t directionalLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	//todo
	wo = -direction;
	s.sp->N = wo;
	s.flags = flags;
	PFLOAT u, v;
	ShirleyDisk(s.s1, s.s2, u, v);
	s.sp->P = position + radius * (u*du + v*dv);
	if(infinite) s.sp->P += direction*worldRadius;
	s.areaPdf = areaPdf;
	s.dirPdf = 1.f;
	return color;
}

light_t *directionalLight_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	point3d_t from(0.0);
	point3d_t dir(0.0, 0.0, 1.0);
	color_t color(1.0);
	CFLOAT power = 1.0;
	float rad = 1.0;
	bool inf = true;
	bool lightEnabled = true;
	bool castShadows = true;
	
	params.getParam("direction",dir);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("infinite", inf);
	params.getParam("light_enabled", lightEnabled);
	params.getParam("cast_shadows", castShadows);
	
	if(!inf)
	{
		if(!params.getParam("from",from))
		{
			if(params.getParam("position",from)) Y_INFO << "DirectionalLight: Deprecated parameter 'position', use 'from' instead" << yendl;
		}
		params.getParam("radius",rad);
	}

	return new directionalLight_t(from, vector3d_t(dir.x, dir.y, dir.z), color, power, inf, rad, lightEnabled, castShadows);
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("directional",directionalLight_t::factory);
}

}

__END_YAFRAY

