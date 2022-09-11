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

#include "accelerator/accelerator.h"
#include "accelerator/accelerator_kdtree_original.h"
#include "accelerator/accelerator_kdtree_multi_thread.h"
#include "accelerator/accelerator_simple_test.h"
#include "common/logger.h"
#include "param/param.h"
#include "integrator/integrator.h"

namespace yafaray {

Accelerator::Params::Params(ParamError &param_error, const ParamMap &param_map)
{
}

ParamMap Accelerator::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE_END;
}

ParamMap Accelerator::getAsParamMap(bool only_non_default) const
{
	ParamMap result{params_.getAsParamMap(only_non_default)};
	result.setParam("type", type().print());
	return result;
}

std::pair<Accelerator *, ParamError> Accelerator::factory(Logger &logger, const std::vector<const Primitive *> &primitives_list, const ParamMap &param_map)
{
	const Type type{ClassMeta::preprocessParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::SimpleTest: return AcceleratorSimpleTest::factory(logger, primitives_list, param_map);
		case Type::KdTreeOriginal: return AcceleratorKdTree::factory(logger, primitives_list, param_map);
		case Type::KdTreeMultiThread: return AcceleratorKdTreeMultiThread::factory(logger, primitives_list, param_map);
		default: return {nullptr, {ParamError::Flags::ErrorWhileCreating}};
	}
}

} //namespace yafaray
