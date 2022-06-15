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

BEGIN_YAFARAY

class Object;

class AcceleratorSimpleTest final : Accelerator
{
	public:
		struct ObjectData
		{
			Bound bound_;
			std::vector<const Primitive *> primitives_;
		};
		static const Accelerator * factory(Logger &logger, const std::vector<const Primitive *> &primitives, const ParamMap &params);

	private:
		AcceleratorSimpleTest(Logger &logger, const std::vector<const Primitive *> &primitives);
		AccelData intersect(const Ray &ray, float t_max) const override;
		AccelData intersectS(const Ray &ray, float t_max, float shadow_bias) const override;
		AccelTsData intersectTs(const Ray &ray, int max_depth, float dist, float shadow_bias, const Camera *camera) const override;
		Bound getBound() const override { return bound_; }
		const std::vector<const Primitive *> &primitives_;
		std::map<const Object *, ObjectData> objects_data_;
		Bound bound_;
};

END_YAFARAY

#endif //YAFARAY_ACCELERATOR_SIMPLE_TEST_H
