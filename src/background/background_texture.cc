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
#include "texture/mipmap_params.h"
#include "param/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "image/image_output.h"

namespace yafaray {

TextureBackground::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
	PARAM_LOAD(rotation_);
	PARAM_LOAD(ibl_blur_);
	PARAM_LOAD(ibl_clamp_sampling_);
	PARAM_ENUM_LOAD(projection_);
	PARAM_LOAD(texture_name_);
}

ParamMap TextureBackground::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(rotation_);
	PARAM_SAVE(ibl_blur_);
	PARAM_SAVE(ibl_clamp_sampling_);
	PARAM_ENUM_SAVE(projection_);
	PARAM_SAVE(texture_name_);
	PARAM_SAVE_END;
}

ParamMap TextureBackground::getAsParamMap(bool only_non_default) const
{
	ParamMap result{ParentClassType_t::getAsParamMap(only_non_default)};
	result.append(params_.getAsParamMap(only_non_default));
	return result;
}

std::pair<std::unique_ptr<Background>, ParamError> TextureBackground::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_error{Params::meta_.check(param_map, {"type"}, {})};
	std::string texname;
	if(param_map.getParam(Params::texture_name_meta_, texname) == ParamError::Flags::ErrorTypeUnknownParam)
	{
		logger.logError(getClassName(), ": No texture given for texture background!");
		return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
	}
	Texture *tex = scene.getTexture(texname);
	if(!tex)
	{
		logger.logError(getClassName(), ": Texture '", texname, "' for textureback not existant!");
		return {nullptr, ParamError{ParamError::Flags::ErrorWhileCreating}};
	}
	auto background{std::make_unique<ThisClassType_t>(logger, param_error, param_map, tex)};
	if(param_error.notOk()) logger.logWarning(param_error.print<ThisClassType_t>(name, {"type"}));
	if(background->ParentClassType_t::params_.ibl_)
	{
		if(background->params_.ibl_blur_ > 0.f)
		{
			logger.logInfo(getClassName(), ": starting background SmartIBL blurring with IBL Blur factor=", background->params_.ibl_blur_);
			tex->generateMipMaps();
			if(logger.isVerbose()) logger.logVerbose(getClassName(), ": background SmartIBL blurring done using mipmaps.");
		}
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = background->ParentClassType_t::params_.ibl_samples_;
		bgp["with_caustic"] = background->ParentClassType_t::params_.with_caustic_;
		bgp["with_diffuse"] = background->ParentClassType_t::params_.with_diffuse_;
		bgp["cast_shadows"] = background->ParentClassType_t::params_.cast_shadows_;
		bgp["abs_intersect"] = false; //this used to be (pr == angular);  but that caused the IBL light to be in the wrong place (see http://www.yafaray.org/node/714) I don't understand why this was set that way, we should keep an eye on this.
		bgp["ibl_clamp_sampling"] = background->params_.ibl_clamp_sampling_;
		if(background->params_.ibl_clamp_sampling_ > 0.f)
		{
			logger.logParams(getClassName(), ": using IBL sampling clamp=", background->params_.ibl_clamp_sampling_);
		}
		auto bglight{Light::factory(logger, scene, "light", bgp).first};
		bglight->setBackground(background.get());
		background->addLight(std::move(bglight));
	}
	return {std::move(background), param_error};
}

TextureBackground::TextureBackground(Logger &logger, ParamError &param_error, const ParamMap &param_map, const Texture *texture) :
		ParentClassType_t{logger, param_error, param_map}, params_{param_error, param_map}, tex_{texture}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	sin_r_ = math::sin(math::num_pi<> * rotation_);
	cos_r_ = math::cos(math::num_pi<> * rotation_);
	if(params_.ibl_blur_ > 0.f)
	{
		with_ibl_blur_ = true;
		ibl_blur_mipmap_level_ = params_.ibl_blur_ * params_.ibl_blur_;
	}
}

Rgb TextureBackground::eval(const Vec3f &dir, bool use_ibl_blur) const
{
	Uv<float> uv;
	if(params_.projection_ == Projection::Angular)
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
	return ParentClassType_t::params_.power_ * ret;
}

} //namespace yafaray
