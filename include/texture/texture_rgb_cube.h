#pragma once
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

#ifndef LIBYAFARAY_TEXTURE_RGB_CUBE_H
#define LIBYAFARAY_TEXTURE_RGB_CUBE_H

#include "texture/texture.h"

namespace yafaray {

/*! RGB cube.
	basically a simple visualization of the RGB color space,
	just goes r in x, g in y and b in z inside the unit cube. */

class RgbCubeTexture final : public Texture
{
		using ThisClassType_t = RgbCubeTexture; using ParentClassType_t = Texture;

	public:
		inline static std::string getClassName() { return "RgbCubeTexture"; }
		static std::pair<std::unique_ptr<Texture>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		explicit RgbCubeTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::RgbCube; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
		} params_;
		Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		float getFloat(const Point3f &p, const MipMapParams *mipmap_params) const override;
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_RGB_CUBE_H
