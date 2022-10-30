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

#ifndef LIBYAFARAY_ACCELERATOR_SIMPLE_TEST_H
#define LIBYAFARAY_ACCELERATOR_SIMPLE_TEST_H

#include "accelerator.h"
#include "geometry/bound.h"
#include <map>

namespace yafaray {

class AcceleratorSimpleTest final : public Accelerator
{
		using ThisClassType_t = AcceleratorSimpleTest; using ParentClassType_t = Accelerator;

	public:
		inline static std::string getClassName() { return "AcceleratorKdTree"; }
		static std::pair<std::unique_ptr<Accelerator>, ParamResult> factory(Logger &logger, const std::vector<const Primitive *> &primitives, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		AcceleratorSimpleTest(Logger &logger, ParamResult &param_result, const std::vector<const Primitive *> &primitives, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::SimpleTest; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
		} params_;
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

		struct ObjectData
		{
			Bound<float> bound_;
			std::vector<const Primitive *> primitives_;
		};
		IntersectData intersect(const Ray &ray, float t_max) const override;
		IntersectData intersectShadow(const Ray &ray, float t_max) const override;
		IntersectData intersectTransparentShadow(const Ray &ray, int max_depth, float dist, const Camera *camera) const override;
		Bound<float> getBound() const override { return bound_; }

		const std::vector<const Primitive *> &primitives_;
		std::map<uintptr_t, ObjectData> object_handles_;
		Bound<float> bound_;
};

} //namespace yafaray

#endif //LIBYAFARAY_ACCELERATOR_SIMPLE_TEST_H
