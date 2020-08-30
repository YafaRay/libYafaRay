/****************************************************************************
 *      textureback.cc: a background using the texture class
 *      This is part of the libYafaRay package
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

#include <yafray_constants.h>
#include <core_api/logging.h>
#include <core_api/environment.h>
#include <core_api/background.h>
#include <core_api/texture.h>
#include <core_api/params.h>
#include <core_api/scene.h>
#include <core_api/light.h>

#include <utilities/sample_utils.h>

BEGIN_YAFRAY

class TextureBackground: public Background
{
	public:
		enum Projection
		{
			Spherical = 0,
			Angular
		};

		TextureBackground(const Texture *texture, Projection proj, float bpower, float rot, bool ibl, float ibl_blur, bool with_caustic);
		virtual Rgb operator()(const Ray &ray, RenderState &state, bool use_ibl_blur = false) const;
		virtual Rgb eval(const Ray &ray, bool use_ibl_blur = false) const;
		virtual ~TextureBackground();
		static Background *factory(ParamMap &, RenderEnvironment &);
		bool hasIbl() { return with_ibl_; }
		bool shootsCaustic() { return shoot_caustic_; }

	protected:
		const Texture *tex_;
		Projection project_;
		float power_;
		float rotation_;
		float sin_r_, cos_r_;
		bool with_ibl_;
		float ibl_blur_mipmap_level_; //Calculated based on the IBL_Blur parameter. As mipmap levels have half size each, this parameter is not linear
		bool shoot_caustic_;
		bool shoot_diffuse_;
};

class ConstantBackground: public Background
{
	public:
		ConstantBackground(Rgb col, bool ibl, bool with_caustic);
		virtual Rgb operator()(const Ray &ray, RenderState &state, bool use_ibl_blur = false) const;
		virtual Rgb eval(const Ray &ray, bool use_ibl_blur = false) const;
		virtual ~ConstantBackground();
		static Background *factory(ParamMap &params, RenderEnvironment &render);
		bool hasIbl() { return with_ibl_; }
		bool shootsCaustic() { return shoot_caustic_; }
	protected:
		Rgb color_;
		bool with_ibl_;
		bool shoot_caustic_;
		bool shoot_diffuse_;
};


TextureBackground::TextureBackground(const Texture *texture, Projection proj, float bpower, float rot, bool ibl, float ibl_blur, bool with_caustic):
		tex_(texture), project_(proj), power_(bpower), with_ibl_(ibl), ibl_blur_mipmap_level_(pow(ibl_blur, 2.f)), shoot_caustic_(with_caustic)
{
	rotation_ = 2.0f * rot / 360.f;
	sin_r_ = fSin__(M_PI * rotation_);
	cos_r_ = fCos__(M_PI * rotation_);
}

TextureBackground::~TextureBackground()
{
	// Empty
}

Rgb TextureBackground::operator()(const Ray &ray, RenderState &state, bool use_ibl_blur) const
{
	return eval(ray, use_ibl_blur);
}

Rgb TextureBackground::eval(const Ray &ray, bool use_ibl_blur) const
{
	float u = 0.f, v = 0.f;

	if(project_ == Angular)
	{
		Point3 dir(ray.dir_);
		dir.x_ = ray.dir_.x_ * cos_r_ + ray.dir_.y_ * sin_r_;
		dir.y_ = ray.dir_.x_ * -sin_r_ + ray.dir_.y_ * cos_r_;
		angmap__(dir, u, v);
	}
	else
	{
		spheremap__(ray.dir_, u, v); // This returns u,v in 0,1 range (useful for bgLight_t)
		// Put u,v in -1,1 range for mapping
		u = 2.f * u - 1.f;
		v = 2.f * v - 1.f;
		u += rotation_;
		if(u > 1.f) u -= 2.f;
	}

	Rgb ret;
	if(use_ibl_blur)
	{
		MipMapParams *mip_map_params = new MipMapParams(ibl_blur_mipmap_level_);
		ret = tex_->getColor(Point3(u, v, 0.f), mip_map_params);
		delete mip_map_params;
		mip_map_params = nullptr;
	}
	else ret = tex_->getColor(Point3(u, v, 0.f));

	float min_component = 1.0e-5f;

	if(ret.r_ < min_component) ret.r_ = min_component;
	if(ret.g_ < min_component) ret.g_ = min_component;
	if(ret.b_ < min_component) ret.b_ = min_component;

	return power_ * ret;
}

Background *TextureBackground::factory(ParamMap &params, RenderEnvironment &render)
{
	Texture *tex = nullptr;
	std::string texname;
	std::string mapping;
	Projection pr = Spherical;
	float power = 1.0, rot = 0.0;
	bool ibl = false;
	float ibl_blur = 0.f;
	float ibl_clamp_sampling = 0.f;
	int ibl_sam = 16;
	bool caust = true;
	bool diffuse = true;
	bool cast_shadows = true;

	if(!params.getParam("texture", texname))
	{
		Y_ERROR << "TextureBackground: No texture given for texture background!" << YENDL;
		return nullptr;
	}
	tex = render.getTexture(texname);
	if(!tex)
	{
		Y_ERROR << "TextureBackground: Texture '" << texname << "' for textureback not existant!" << YENDL;
		return nullptr;
	}
	if(params.getParam("mapping", mapping))
	{
		if(mapping == "probe" || mapping == "angular") pr = Angular;
	}
	params.getParam("ibl", ibl);
	params.getParam("smartibl_blur", ibl_blur);
	params.getParam("ibl_clamp_sampling", ibl_clamp_sampling);
	params.getParam("ibl_samples", ibl_sam);
	params.getParam("power", power);
	params.getParam("rotation", rot);
	params.getParam("with_caustic", caust);
	params.getParam("with_diffuse", diffuse);
	params.getParam("cast_shadows", cast_shadows);

	Background *tex_bg = new TextureBackground(tex, pr, power, rot, ibl, ibl_blur, caust);

	if(ibl)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = ibl_sam;
		bgp["with_caustic"] = caust;
		bgp["with_diffuse"] = diffuse;
		bgp["abs_intersect"] = false; //this used to be (pr == angular);  but that caused the IBL light to be in the wrong place (see http://www.yafaray.org/node/714) I don't understand why this was set that way, we should keep an eye on this.
		bgp["cast_shadows"] = cast_shadows;

		if(ibl_blur > 0.f)
		{
			Y_INFO << "TextureBackground: starting background SmartIBL blurring with IBL Blur factor=" << ibl_blur << YENDL;
			tex->generateMipMaps();
			Y_VERBOSE << "TextureBackground: background SmartIBL blurring done using mipmaps." << YENDL;
		}

		Light *bglight = render.createLight("textureBackground_bgLight", bgp);

		bglight->setBackground(tex_bg);

		if(ibl_clamp_sampling > 0.f)
		{
			Y_INFO << "TextureBackground: using IBL sampling clamp=" << ibl_clamp_sampling << YENDL;

			bglight->setClampIntersect(ibl_clamp_sampling);
		}

		if(bglight) render.getScene()->addLight(bglight);
	}

	return tex_bg;
}

/* ========================================
/ minimalistic background...
/ ========================================= */

ConstantBackground::ConstantBackground(Rgb col, bool ibl, bool with_caustic) : color_(col), with_ibl_(ibl), shoot_caustic_(with_caustic)
{
	// Empty
}
ConstantBackground::~ConstantBackground()
{
	// Empty
}

Rgb ConstantBackground::operator()(const Ray &ray, RenderState &state, bool use_ibl_blur) const
{
	return color_;
}

Rgb ConstantBackground::eval(const Ray &ray, bool use_ibl_blur) const
{
	return color_;
}

Background *ConstantBackground::factory(ParamMap &params, RenderEnvironment &render)
{
	Rgb col(0.f);
	float power = 1.0;
	int ibl_sam = 16;
	bool ibl = false;
	bool cast_shadows = true;
	bool caus = true;
	bool diff = true;

	params.getParam("color", col);
	params.getParam("power", power);
	params.getParam("ibl", ibl);
	params.getParam("ibl_samples", ibl_sam);
	params.getParam("cast_shadows", cast_shadows);
	params.getParam("with_caustic", caus);
	params.getParam("with_diffuse", diff);

	Background *const_bg = new ConstantBackground(col * power, ibl, true);

	if(ibl)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = ibl_sam;
		bgp["with_caustic"] = caus;
		bgp["with_diffuse"] = diff;
		bgp["cast_shadows"] = cast_shadows;

		Light *bglight = render.createLight("constantBackground_bgLight", bgp);

		bglight->setBackground(const_bg);

		if(bglight) render.getScene()->addLight(bglight);
	}

	return const_bg;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("textureback", TextureBackground::factory);
		render.registerFactory("constant", ConstantBackground::factory);
	}

}
END_YAFRAY
