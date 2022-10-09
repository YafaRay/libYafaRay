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

#ifndef LIBYAFARAY_BACKGROUND_H
#define LIBYAFARAY_BACKGROUND_H

#include "color/color.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"
#include <memory>
#include <vector>

namespace yafaray {

class ParamMap;
class Scene;
class Light;
class Rgb;
class Logger;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;

class Background
{
	public:
		inline static std::string getClassName() { return "Background"; }
		static std::pair<std::unique_ptr<Background>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		explicit Background(Logger &logger, ParamError &param_error, const ParamMap &param_map);
		virtual ~Background();
		Rgb operator()(const Vec3f &dir) const { return operator()(dir, false); }
		virtual Rgb operator()(const Vec3f &dir, bool use_ibl_blur) const { return eval(dir, use_ibl_blur); }
		Rgb eval(const Vec3f &dir) const { return eval(dir, false); }
		virtual Rgb eval(const Vec3f &dir, bool use_ibl_blur) const = 0;
		void addLight(std::unique_ptr<Light> light);
		std::vector<Light *> getLights() const;

	protected:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { None, DarkSky, Gradient, SunSky, Texture, Constant };
			inline static const EnumMap<ValueType_t> map_{{
					{"darksky", DarkSky, ""},
					{"gradientback", Gradient, ""},
					{"sunsky", SunSky, ""},
					{"textureback", Texture, ""},
					{"constant", Constant, ""},
				}};
		};
		[[nodiscard]] virtual Type type() const = 0;
		const struct Params
		{
			PARAM_INIT;
			PARAM_DECL(float, power_, 1.f, "power", "");
			PARAM_DECL(bool , ibl_, false, "ibl", "");
			PARAM_DECL(int, ibl_samples_, 16, "ibl_samples", "");
			PARAM_DECL(bool, with_caustic_, true, "with_caustic", "");
			PARAM_DECL(bool, with_diffuse_, true, "with_diffuse", "");
			PARAM_DECL(bool, cast_shadows_, true, "cast_shadows", "");
		} params_;
		std::vector<std::unique_ptr<Light>> lights_;
		Logger &logger_;
};

} //namespace yafaray

#endif // LIBYAFARAY_BACKGROUND_H
