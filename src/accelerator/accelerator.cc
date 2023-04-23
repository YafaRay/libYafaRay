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

namespace yafaray {

std::map<std::string, const ParamMeta *> Accelerator::Params::getParamMetaMap()
{
	return {};
}

Accelerator::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
}

ParamMap Accelerator::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	return param_map;
}

std::pair<std::unique_ptr<Accelerator>, ParamResult> Accelerator::factory(Logger &logger, const RenderControl *render_control, const std::vector<const Primitive *> &primitives_list, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("** " + getClassName() + "::factory 'raw' ParamMap contents:\n" + param_map.logContents());
	const auto type{class_meta::getTypeFromParamMap<Type>(logger, getClassName(), param_map)};
	switch(type.value())
	{
		case Type::SimpleTest: return AcceleratorSimpleTest::factory(logger, render_control, primitives_list, param_map);
		case Type::KdTreeOriginal: return AcceleratorKdTree::factory(logger, render_control, primitives_list, param_map);
		case Type::KdTreeMultiThread: return AcceleratorKdTreeMultiThread::factory(logger, render_control, primitives_list, param_map);
		default: return {nullptr, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
	}
}

} //namespace yafaray
