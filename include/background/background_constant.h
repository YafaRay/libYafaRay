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
		using ThisClassType_t = ConstantBackground; using ParentClassType_t = Background;

	public:
		inline static std::string getClassName() { return "ConstantBackground"; }
		static std::pair<std::unique_ptr<Background>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &params);
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		ConstantBackground(Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		std::vector<std::pair<std::string, ParamMap>> getRequestedIblLights() const override;

	private:
		[[nodiscard]] Type type() const override { return Type::Constant; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(Rgb, color_, Rgb{0.f}, "color", "");
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		Rgb eval(const Vec3f &dir, bool use_ibl_blur) const override;
		static std::string lightName(){ return getClassName() + "::light"; }

		const Rgb color_{params_.color_ * ParentClassType_t::params_.power_};
};

} //namespace yafaray

#endif // LIBYAFARAY_BACKGROUND_CONSTANT_H