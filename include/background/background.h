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
class Texture;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
template <typename T> class Items;

class Background
{
	protected: struct Type;
	public:
		inline static std::string getClassName() { return "Background"; }
		[[nodiscard]] virtual Type type() const = 0;
		static std::pair<std::unique_ptr<Background>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &param_map, const Items<Texture> &textures);
		[[nodiscard]] virtual std::map<std::string, const ParamMeta *> getParamMetaMap() const = 0;
		[[nodiscard]] std::string exportToString(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const;
		[[nodiscard]] virtual ParamMap getAsParamMap(bool only_non_default) const;
		Background(Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		virtual ~Background() = default; //Needed for proper destruction of derived classes
		Rgb operator()(const Vec3f &dir) const { return operator()(dir, false); }
		virtual Rgb operator()(const Vec3f &dir, bool use_ibl_blur) const { return eval(dir, use_ibl_blur); }
		Rgb eval(const Vec3f &dir) const { return eval(dir, false); }
		virtual Rgb eval(const Vec3f &dir, bool use_ibl_blur) const = 0;
		virtual bool usesIblBlur() const { return false; }
		virtual size_t getTextureId() const { return math::invalid<size_t>; }
		virtual std::vector<std::pair<std::string, ParamMap>> getRequestedIblLights() const = 0;

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
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(float, power_, 1.f, "power", "");
			PARAM_DECL(bool , ibl_, false, "ibl", "");
			PARAM_DECL(int, ibl_samples_, 16, "ibl_samples", "");
			PARAM_DECL(bool, with_caustic_, true, "with_caustic", "");
			PARAM_DECL(bool, with_diffuse_, true, "with_diffuse", "");
			PARAM_DECL(bool, cast_shadows_, true, "cast_shadows", "");
		} params_;
		Logger &logger_;
};

} //namespace yafaray

#endif // LIBYAFARAY_BACKGROUND_H
