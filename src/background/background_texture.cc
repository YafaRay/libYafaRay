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

#include "background/background_texture.h"
#include "texture/texture_image.h"
#include "common/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "image/image_output.h"

namespace yafaray {

TextureBackground::TextureBackground(Logger &logger, const Texture *texture, Projection proj, float bpower, float rot, float ibl_blur) :
		Background(logger), tex_(texture), project_(proj), power_(bpower), ibl_blur_mipmap_level_(math::pow(ibl_blur, 2.f))
{
	rotation_ = 2.f * rot / 360.f;
	sin_r_ = math::sin(math::num_pi<> * rotation_);
	cos_r_ = math::cos(math::num_pi<> * rotation_);
	if(ibl_blur > 0.f)
	{
		with_ibl_blur_ = true;
		ibl_blur_mipmap_level_ = ibl_blur * ibl_blur;
	}
}

Rgb TextureBackground::eval(const Vec3f &dir, bool use_ibl_blur) const
{
	Uv<float> uv;
	if(project_ == Angular)
	{
		const Point3f p {{
				dir[Axis::X] * cos_r_ + dir[Axis::Y] * sin_r_,
				dir[Axis::X] * -sin_r_ + dir[Axis::Y] * cos_r_,
				dir[Axis::Z]
		}};
		uv = Texture::angMap(p);
	}
	else
	{
		uv = Texture::sphereMap(static_cast<Point3f>(dir)); // This returns u,v in 0,1 range (useful for bgLight_t)
		// Put u,v in -1,1 range for mapping
		uv.u_ = 2.f * uv.u_ - 1.f;
		uv.v_ = 2.f * uv.v_ - 1.f;
		uv.u_ += rotation_;
		if(uv.u_ > 1.f) uv.u_ -= 2.f;
	}
	Rgb ret;
	if(with_ibl_blur_ && use_ibl_blur)
	{
		const MipMapParams mip_map_params {ibl_blur_mipmap_level_};
		ret = tex_->getColor({{uv.u_, uv.v_, 0.f}}, &mip_map_params);
	}
	else ret = tex_->getColor({{uv.u_, uv.v_, 0.f}});

	const float min_component = 1.0e-5f;
	if(ret.r_ < min_component) ret.r_ = min_component;
	if(ret.g_ < min_component) ret.g_ = min_component;
	if(ret.b_ < min_component) ret.b_ = min_component;
	return power_ * ret;
}

const Background * TextureBackground::factory(Logger &logger, Scene &scene, const std::string &name, const ParamMap &params)
{
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
		logger.logError("TextureBackground: No texture given for texture background!");
		return nullptr;
	}

	Texture *tex = scene.getTexture(texname);
	if(!tex)
	{
		logger.logError("TextureBackground: Texture '", texname, "' for textureback not existant!");
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

	auto tex_bg = new TextureBackground(logger, tex, pr, power, rot, ibl_blur);

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
			logger.logInfo("TextureBackground: starting background SmartIBL blurring with IBL Blur factor=", ibl_blur);
			tex->generateMipMaps();
			if(logger.isVerbose()) logger.logVerbose("TextureBackground: background SmartIBL blurring done using mipmaps.");
		}

		Light *bglight = scene.createLight("textureBackground_bgLight", std::move(bgp));

		bglight->setBackground(tex_bg);

		if(ibl_clamp_sampling > 0.f)
		{
			logger.logInfo("TextureBackground: using IBL sampling clamp=", ibl_clamp_sampling);

			bglight->setClampIntersect(ibl_clamp_sampling);
		}
	}

	return tex_bg;
}

} //namespace yafaray
