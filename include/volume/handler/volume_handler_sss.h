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

#ifndef LIBYAFARAY_VOLUME_HANDLER_SSS_H
#define LIBYAFARAY_VOLUME_HANDLER_SSS_H

#include "volume_handler_beer.h"

namespace yafaray {

class SssVolumeHandler final : public BeerVolumeHandler
{
		using ThisClassType_t = SssVolumeHandler; using ParentClassType_t = BeerVolumeHandler;

	public:
		inline static std::string getClassName() { return "SssVolumeHandler"; }
		static std::pair<std::unique_ptr<VolumeHandler>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		SssVolumeHandler(Logger &logger, ParamError &param_error, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::Sss; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(Rgb, scatter_col_, Rgb{0.8f}, "scatter_col", "");
		} params_;
		bool scatter(const Ray &ray, Ray &s_ray, PSample &s) const override;
};

} //namespace yafaray

#endif // LIBYAFARAY_VOLUME_HANDLER_SSS_H