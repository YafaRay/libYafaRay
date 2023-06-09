/****************************************************************************
 *      This is part of the libYafaRay package
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

#include "texture/texture_rgb_cube.h"
#include "param/param.h"
#include "scene/scene.h"

namespace yafaray {

//-----------------------------------------------------------------------------------------
/* even simpler RGB cube, goes r in x, g in y and b in z inside the unit cube.  */
//-----------------------------------------------------------------------------------------

std::map<std::string, const ParamMeta *> RgbCubeTexture::Params::getParamMetaMap()
{
	auto param_meta_map{ParentClassType_t::Params::getParamMetaMap()};
	return param_meta_map;
}

RgbCubeTexture::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
}

ParamMap RgbCubeTexture::getAsParamMap(bool only_non_default) const
{
	auto param_map{ParentClassType_t::getAsParamMap(only_non_default)};
	param_map.setParam("type", type().print());
	return param_map;
}

std::pair<std::unique_ptr<Texture>, ParamResult> RgbCubeTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	auto param_result{class_meta::check<Params>(param_map, {"type"}, {"ramp_item_"})};
	auto texture {std::make_unique<RgbCubeTexture>(logger, param_result, param_map, scene.getTextures())};
	if(param_result.notOk()) logger.logWarning(param_result.print<ThisClassType_t>(name, {"type"}));
	return {std::move(texture), param_result};
}

RgbCubeTexture::RgbCubeTexture(Logger &logger, ParamResult &param_result, const ParamMap &param_map, const Items <Texture> &textures) : ParentClassType_t{logger, param_result, param_map, textures}, params_{param_result, param_map}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
}

Rgba RgbCubeTexture::getColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	Rgba col = Rgba(p[Axis::X], p[Axis::Y], p[Axis::Z]);
	col.clampRgb01();
	if(adjustments_set_) return applyAdjustments(col);
	else return col;
}

float RgbCubeTexture::getFloat(const Point3f &p, const MipMapParams *mipmap_params) const
{
	Rgb col = Rgb(p[Axis::X], p[Axis::Y], p[Axis::Z]);
	col.clampRgb01();
	return applyIntensityContrastAdjustments(col.energy());
}

} //namespace yafaray
