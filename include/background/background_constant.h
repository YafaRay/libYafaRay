#pragma once
/****************************************************************************
 *      background_constant.h: a background using a constant color
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

#ifndef LIBYAFARAY_BACKGROUND_CONSTANT_H
#define LIBYAFARAY_BACKGROUND_CONSTANT_H

#include <memory>
#include "background.h"
#include "color/color.h"

namespace yafaray {

class ConstantBackground final : public Background
{
	public:
		inline static std::string getClassName() { return "ConstantBackground"; }
		static std::pair<std::unique_ptr<Background>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		ConstantBackground(Logger &logger, ParamError &param_error, const ParamMap &param_map);

	private:
		using ThisClassType_t = ConstantBackground;
		using ParentClassType_t = Background;
		[[nodiscard]] Type type() const override { return Type::Constant; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Rgb, color_, Rgb{0.f}, "color", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

		Rgb eval(const Vec3f &dir, bool use_ibl_blur) const override;

		Rgb color_;
};

} //namespace yafaray

#endif // LIBYAFARAY_BACKGROUND_CONSTANT_H