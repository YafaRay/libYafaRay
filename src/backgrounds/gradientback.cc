/****************************************************************************
 * 			gradientback.cc: a background using a simple color gradient
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
 
#include <yafray_config.h>

#include <core_api/environment.h>
#include <core_api/background.h>
#include <core_api/light.h>
//#include <utilities/sample_utils.h>
#include <lights/bglight.h>

__BEGIN_YAFRAY

class gradientBackground_t: public background_t
{
	public:
		enum PROJECTION { spherical=0, angular };
		gradientBackground_t(color_t gzcol, color_t ghcol, color_t szcol, color_t shcol, bool ibl, int samples);
		virtual color_t operator() (const ray_t &ray, renderState_t &state, bool filtered=false) const;
		virtual color_t eval(const ray_t &ray, bool filtered=false) const;
		virtual light_t* getLight() const { return envLight; }
		virtual ~gradientBackground_t();
		static background_t *factory(paraMap_t &,renderEnvironment_t &);
	protected:

		color_t gzenith,  ghoriz, szenith, shoriz;
		
		//bool ibl; //!< indicate wether to do image based lighting
		light_t *envLight;
};

gradientBackground_t::gradientBackground_t(color_t gzcol, color_t ghcol, color_t szcol, color_t shcol, bool ibl, int samples):
						gzenith(gzcol),  ghoriz(ghcol), szenith(szcol), shoriz(shcol), envLight(0)
{
	if(ibl)
	{
		envLight = new bgLight_t(this, samples);
	}
}

gradientBackground_t::~gradientBackground_t()
{
	if(envLight) delete envLight;
}

color_t gradientBackground_t::operator() (const ray_t &ray, renderState_t &state, bool filtered) const
{
	return eval(ray);
}

color_t gradientBackground_t::eval(const ray_t &ray, bool filtered) const
{
	color_t color;
	CFLOAT blend = ray.dir.z;
	if(blend >= 0.f)
	{
		color = blend*szenith + (1.f-blend)*shoriz;
	}
	else 
	{
		blend = -blend;
		color = blend*gzenith + (1.f-blend)*ghoriz;
	}
	if(color.minimum() < 0.000001) color = color_t(0.00001);
	return color;
}

background_t* gradientBackground_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	color_t gzenith,  ghoriz, szenith(0.4f, 0.5f, 1.f), shoriz(1.f);
	float p = 1.0;
	bool bgl = false;
	int bglSam = 8; //standard wild guess :P
	params.getParam("horizon_color", shoriz);
	params.getParam("zenith_color", szenith);
	gzenith = szenith;
	ghoriz = shoriz;
	params.getParam("horizon_ground_color", ghoriz);
	params.getParam("zenith_ground_color", gzenith);
	params.getParam("ibl", bgl);
	params.getParam("ibl_samples", bglSam);
	params.getParam("power", p);
	
	return new gradientBackground_t(gzenith*p,  ghoriz*p, szenith*p, shoriz*p, bgl, bglSam);
}

extern "C"
{
	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("gradientback",gradientBackground_t::factory);
	}

}
__END_YAFRAY
