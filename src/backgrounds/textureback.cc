/****************************************************************************
 * 			textureback.cc: a background using the texture class
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
#include <core_api/texture.h>
#include <core_api/params.h>
#include <core_api/scene.h>
#include <core_api/light.h>

#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

class textureBackground_t: public background_t
{
	public:
		enum PROJECTION
		{
			spherical = 0,
			angular
		};

		textureBackground_t(const texture_t *texture, PROJECTION proj, float bpower, float rot);
		virtual color_t operator() (const ray_t &ray, renderState_t &state, bool filtered=false) const;
		virtual color_t eval(const ray_t &ray, bool filtered=false) const;
		virtual ~textureBackground_t();
		static background_t *factory(paraMap_t &,renderEnvironment_t &);

	protected:
		const texture_t *tex;
		PROJECTION project;
		float power;
		float rotation;
		float sin_r, cos_r;
		bool shootCaustic;
		bool shootDiffuse;
};

class constBackground_t: public background_t
{
	public:
		constBackground_t(color_t col);
		virtual color_t operator() (const ray_t &ray, renderState_t &state, bool filtered=false) const;
		virtual color_t eval(const ray_t &ray, bool filtered=false) const;
		virtual ~constBackground_t();
		static background_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		color_t color;
};


textureBackground_t::textureBackground_t(const texture_t *texture, PROJECTION proj, float bpower, float rot):
	tex(texture), project(proj), power(bpower)
{
	rotation = 2.0f * rot / 360.f;
	sin_r = fSin(M_PI*rotation);
	cos_r = fCos(M_PI*rotation);
}

textureBackground_t::~textureBackground_t()
{
	// Empty
}

color_t textureBackground_t::operator() (const ray_t &ray, renderState_t &state, bool filtered) const
{
	return eval(ray);
}

color_t textureBackground_t::eval(const ray_t &ray, bool filtered) const
{
	float u = 0.f, v = 0.f;
	
	if (project == angular)
	{
		point3d_t dir(ray.dir);
		dir.x = ray.dir.x * cos_r + ray.dir.y * sin_r;
		dir.y = ray.dir.x * -sin_r + ray.dir.y * cos_r;
		angmap(dir, u, v);
	}
	else
	{
		spheremap(ray.dir, u, v); // This returns u,v in 0,1 range (useful for bgLight_t)
		// Put u,v in -1,1 range for mapping
		u = 2.f * u - 1.f;
		v = 2.f * v - 1.f;
		u += rotation;
		if (u > 1.f) u -= 2.f;
	}
	
	color_t ret = tex->getColor(point3d_t(u, v, 0.f));
	
	if(ret.minimum() < 1e-6f) ret = color_t(1e-5f);
	
	return power * ret;
}

background_t* textureBackground_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	const texture_t *tex=0;
	const std::string *texname=0;
	const std::string *mapping=0;
	PROJECTION pr = spherical;
	float power = 1.0, rot=0.0;
	bool IBL = false;
	int IBL_sam = 16;
	bool caust = true;
	bool diffuse = true;
	
	if( !params.getParam("texture", texname) )
	{
		Y_ERROR << "TextureBackground: No texture given for texture background!" << yendl;
		return NULL;
	}
	tex = render.getTexture(*texname);
	if( !tex )
	{
		Y_ERROR << "TextureBackground: Texture '" << *texname << "' for textureback not existant!" << yendl;
		return NULL;
	}
	if( params.getParam("mapping", mapping) )
	{
		if(*mapping == "probe" || *mapping == "angular") pr = angular;
	}
	params.getParam("ibl", IBL);
	params.getParam("ibl_samples", IBL_sam);
	params.getParam("power", power);
	params.getParam("rotation", rot);
	params.getParam("with_caustic", caust);
	params.getParam("with_diffuse", diffuse);
	
	background_t *texBG = new textureBackground_t(tex, pr, power, rot);
	
	if(IBL)
	{
		paraMap_t bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = IBL_sam;
		bgp["shoot_caustics"] = caust;
		bgp["shoot_diffuse"] = diffuse;
		bgp["abs_intersect"] = (pr == angular);
		
		light_t *bglight = render.createLight("textureBackground_bgLight", bgp);
		
		bglight->setBackground(texBG);
		
		if(bglight) render.getScene()->addLight(bglight);
	}

	return texBG;
}

/* ========================================
/ minimalistic background...
/ ========================================= */

constBackground_t::constBackground_t(color_t col) : color(col)
{
	// Empty
}
constBackground_t::~constBackground_t()
{
	// Empty
}

color_t constBackground_t::operator() (const ray_t &ray, renderState_t &state, bool filtered) const
{
	return color;
}

color_t constBackground_t::eval(const ray_t &ray, bool filtered) const
{
	return color;
}

background_t* constBackground_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	color_t col(0.f);
	float power = 1.0;
	int IBL_sam = 16;
	bool IBL = false;
	
	params.getParam("color", col);
	params.getParam("power", power);
	params.getParam("ibl", IBL);
	params.getParam("ibl_samples", IBL_sam);
	
	background_t *constBG = new constBackground_t(col*power);
	
	if(IBL)
	{
		paraMap_t bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = IBL_sam;
		bgp["shoot_caustics"] = false;
		bgp["shoot_diffuse"] = true;
		
		light_t *bglight = render.createLight("constantBackground_bgLight", bgp);
		
		bglight->setBackground(constBG);
		
		if(bglight) render.getScene()->addLight(bglight);
	}

	return constBG;
}

extern "C"
{
	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("textureback",textureBackground_t::factory);
		render.registerFactory("constant", constBackground_t::factory);
	}

}
__END_YAFRAY
