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

#ifndef YAFARAY_ACCELERATOR_SIMPLE_TEST_H
#define YAFARAY_ACCELERATOR_SIMPLE_TEST_H

#include "accelerator.h"
#include "geometry/bound.h"
#include <map>

namespace yafaray {

class Object;

class AcceleratorSimpleTest final : Accelerator
{
	public:
		inline static std::string getClassName() { return "AcceleratorKdTree"; }
		static std::pair<Accelerator *, ParamError> factory(Logger &logger, const std::vector<const Primitive *> &primitives, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }

	private:
		[[nodiscard]] Type type() const override { return Type::SimpleTest; }
		const struct Params
		{
			PARAM_INIT_PARENT(Accelerator);
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

		struct ObjectData
		{
			Bound<float> bound_;
			std::vector<const Primitive *> primitives_;
		};
		AcceleratorSimpleTest(Logger &logger, ParamError &param_error, const std::vector<const Primitive *> &primitives, const ParamMap &param_map);
		IntersectData intersect(const Ray &ray, float t_max) const override;
		IntersectData intersectShadow(const Ray &ray, float t_max) const override;
		IntersectData intersectTransparentShadow(const Ray &ray, int max_depth, float dist, const Camera *camera) const override;
		Bound<float> getBound() const override { return bound_; }

		const std::vector<const Primitive *> &primitives_;
		std::map<const Object *, ObjectData> objects_data_;
		Bound<float> bound_;
};

} //namespace yafaray

#endif //YAFARAY_ACCELERATOR_SIMPLE_TEST_H
