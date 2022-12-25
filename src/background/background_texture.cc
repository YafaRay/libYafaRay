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

TextureBackground::Params::Params(ParamResult &param_result, const ParamMap &param_map)
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

std::pair<std::unique_ptr<Background>, ParamResult> TextureBackground::factory(Logger &logger, const std::string &name, const Scene &scene, const ParamMap &param_map)
{
	auto param_result{Params::meta_.check(param_map, {"type"}, {})};
	std::string texname;
	if(param_map.getParam(Params::texture_name_meta_, texname) == YAFARAY_RESULT_ERROR_TYPE_UNKNOWN_PARAM)
	{
		logger.logError(getClassName(), ": No texture given for texture background!");
		return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
	auto [tex, tex_id, tex_result]{scene.getTexture(texname)};
	if(!tex_result.isOk())
	{
		logger.logError(getClassName(), ": Texture '", texname, "' does not exist!");
		return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
	auto background{std::make_unique<ThisClassType_t>(logger, param_result, param_map, tex_id, scene.getTextures())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(background), param_result};
}

TextureBackground::TextureBackground(Logger &logger, ParamResult &param_result, const ParamMap &param_map, size_t texture_id, const Items <Texture> &textures) :
		ParentClassType_t{logger, param_result, param_map}, params_{param_result, param_map}, texture_id_{texture_id}, textures_{textures}
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
		ret = textures_.getById(texture_id_).first->getColor({{uv.u_, uv.v_, 0.f}}, &mip_map_params);
	}
	else ret = textures_.getById(texture_id_).first->getColor({{uv.u_, uv.v_, 0.f}});

	const float min_component = 1.0e-5f;
	if(ret.r_ < min_component) ret.r_ = min_component;
	if(ret.g_ < min_component) ret.g_ = min_component;
	if(ret.b_ < min_component) ret.b_ = min_component;
	return ParentClassType_t::params_.power_ * ret;
}

std::vector<std::pair<std::string, ParamMap>> TextureBackground::getRequestedIblLights() const
{
	if(ParentClassType_t::params_.ibl_)
	{
		ParamMap bgp;
		bgp["type"] = std::string("bglight");
		bgp["samples"] = ParentClassType_t::params_.ibl_samples_;
		bgp["with_caustic"] = ParentClassType_t::params_.with_caustic_;
		bgp["with_diffuse"] = ParentClassType_t::params_.with_diffuse_;
		bgp["cast_shadows"] = ParentClassType_t::params_.cast_shadows_;
		bgp["abs_intersect"] = false; //this used to be (pr == angular);  but that caused the IBL light to be in the wrong place (see http://www.yafaray.org/node/714) I don't understand why this was set that way, we should keep an eye on this.
		bgp["ibl_clamp_sampling"] = params_.ibl_clamp_sampling_;
		return {{ThisClassType_t::lightName(), std::move(bgp)}};
	}
	else return {};
}

} //namespace yafaray
