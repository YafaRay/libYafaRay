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

		textureBackground_t(const texture_t *texture, PROJECTION proj, float bpower, float rot, bool ibl, float ibl_blur, bool with_caustic);
		virtual color_t operator() (const ray_t &ray, renderState_t &state, bool use_ibl_blur=false) const;
		virtual color_t eval(const ray_t &ray, bool use_ibl_blur=false) const;
		virtual ~textureBackground_t();
		static background_t *factory(paraMap_t &,renderEnvironment_t &);
		bool hasIBL() { return withIBL; }
		bool shootsCaustic() { return shootCaustic; }
		
	protected:
		const texture_t *tex;
		PROJECTION project;
		float power;
		float rotation;
		float sin_r, cos_r;
		bool withIBL;
		float IBL_Blur_mipmap_level; //Calculated based on the IBL_Blur parameter. As mipmap levels have half size each, this parameter is not linear
		bool shootCaustic;
		bool shootDiffuse;
};

class constBackground_t: public background_t
{
	public:
		constBackground_t(color_t col, bool ibl, bool with_caustic);
		virtual color_t operator() (const ray_t &ray, renderState_t &state, bool use_ibl_blur=false) const;
		virtual color_t eval(const ray_t &ray, bool use_ibl_blur=false) const;
		virtual ~constBackground_t();
		static background_t *factory(paraMap_t &params,renderEnvironment_t &render);
		bool hasIBL() { return withIBL; }
		bool shootsCaustic() { return shootCaustic; }
	protected:
		color_t color;
		bool withIBL;
		bool shootCaustic;
		bool shootDiffuse;
};


textureBackground_t::textureBackground_t(const texture_t *texture, PROJECTION proj, float bpower, float rot, bool ibl, float ibl_blur, bool with_caustic):
	tex(texture), project(proj), power(bpower), withIBL(ibl), IBL_Blur_mipmap_level(pow(ibl_blur, 2.f)), shootCaustic(with_caustic)
{
	rotation = 2.0f * rot / 360.f;
	sin_r = fSin(M_PI*rotation);
	cos_r = fCos(M_PI*rotation);
}

textureBackground_t::~textureBackground_t()
{
	// Empty
}

color_t textureBackground_t::operator() (const ray_t &ray, renderState_t &state, bool use_ibl_blur) const
{
	return eval(ray, use_ibl_blur);
}

color_t textureBackground_t::eval(const ray_t &ray, bool use_ibl_blur) const
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
	
	color_t ret;
	if(use_ibl_blur)
	{
		mipMapParams_t * mipMapParams = new mipMapParams_t(IBL_Blur_mipmap_level);
		ret = tex->getColor(point3d_t(u, v, 0.f), mipMapParams);
		delete mipMapParams;
		mipMapParams = nullptr;
	}
	else ret = tex->getColor(point3d_t(u, v, 0.f));
	
	float minComponent = 1.0e-5f;
	
	if(ret.R < minComponent) ret.R = minComponent;
	if(ret.G < minComponent) ret.G = minComponent;
	if(ret.B < minComponent) ret.B = minComponent;
	
	return power * ret;
}

background_t* textureBackground_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	texture_t *tex=nullptr;
	const std::string *texname=nullptr;
	const std::string *mapping=nullptr;
	PROJECTION pr = spherical;
	float power = 1.0, rot=0.0;
	bool IBL = false;
	float IBL_blur = 0.f;
	float IBL_clamp_sampling = 0.f;
	int IBL_sam = 16;
	bool caust = true;
	bool diffuse = true;
	bool castShadows = true;
	
	if( !params.getParam("texture", texname) )
	{
		Y_ERROR << "TextureBackground: No texture given for texture background!" << yendl;
		return nullptr;
	}
	tex = render.getTexture(*texname);
	if( !tex )
	{
		Y_ERROR << "TextureBackground: Texture '" << *texname << "' for textureback not existant!" << yendl;
		return nullptr;
	}
	if( params.getParam("mapping", mapping) )
	{
		if(*mapping == "probe" || *mapping == "angular") pr = angular;
	}
	params.getParam("ibl", IBL);
	params.getParam("smartibl_blur", IBL_blur);
	params.getParam("ibl_clamp_sampling", IBL_clamp_sampling);
	params.getParam("ibl_samples", IBL_sam);
	params.getParam("power", power);
	params.getParam("rotation", rot);
	params.getParam("with_caustic", caust);
	params.getParam("with_diffuse", diffuse);
	params.getParam("cast_shadows", castShadows);
	
	background_t *texBG = new textureBackground_t(tex, pr, power, rot, IBL, IBL_blur, caust);
	
	if(IBL)
	{
		paraMap_t bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = IBL_sam;
		bgp["with_caustic"] = caust;
		bgp["with_diffuse"] = diffuse;
		bgp["abs_intersect"] = false; //this used to be (pr == angular);  but that caused the IBL light to be in the wrong place (see http://www.yafaray.org/node/714) I don't understand why this was set that way, we should keep an eye on this.
		bgp["cast_shadows"] = castShadows;
				
		if(IBL_blur > 0.f)
		{
			Y_INFO << "TextureBackground: starting background SmartIBL blurring with IBL Blur factor=" << IBL_blur << yendl;
			tex->generateMipMaps();
			Y_VERBOSE << "TextureBackground: background SmartIBL blurring done using mipmaps." << yendl;
		}
		
		light_t *bglight = render.createLight("textureBackground_bgLight", bgp);
		
		bglight->setBackground(texBG);
		
		if(IBL_clamp_sampling > 0.f)
		{
			Y_INFO << "TextureBackground: using IBL sampling clamp=" << IBL_clamp_sampling << yendl;
			
			bglight->setClampIntersect(IBL_clamp_sampling);
		}
		
		if(bglight) render.getScene()->addLight(bglight);
	}

	return texBG;
}

/* ========================================
/ minimalistic background...
/ ========================================= */

constBackground_t::constBackground_t(color_t col, bool ibl, bool with_caustic) : color(col), withIBL(ibl), shootCaustic(with_caustic)
{
	// Empty
}
constBackground_t::~constBackground_t()
{
	// Empty
}

color_t constBackground_t::operator() (const ray_t &ray, renderState_t &state, bool use_ibl_blur) const
{
	return color;
}

color_t constBackground_t::eval(const ray_t &ray, bool use_ibl_blur) const
{
	return color;
}

background_t* constBackground_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	color_t col(0.f);
	float power = 1.0;
	int IBL_sam = 16;
	bool IBL = false;
	bool castShadows = true;
	bool caus = true;
	bool diff = true;
	
	params.getParam("color", col);
	params.getParam("power", power);
	params.getParam("ibl", IBL);
	params.getParam("ibl_samples", IBL_sam);
	params.getParam("cast_shadows", castShadows);
	params.getParam("with_caustic", caus);
	params.getParam("with_diffuse", diff);
	
	background_t *constBG = new constBackground_t(col*power, IBL, true);
	
	if(IBL)
	{
		paraMap_t bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = IBL_sam;
		bgp["with_caustic"] = caus;
		bgp["with_diffuse"] = diff;
		bgp["cast_shadows"] = castShadows;
		
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
