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
#include "common/param.h"
#include "render/render_data.h"
#include "integrator/integrator.h"

namespace yafaray {

const Accelerator * Accelerator::factory(Logger &logger, const std::vector<const Primitive *> &primitives_list, const ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Accelerator");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	const Accelerator *accelerator = nullptr;
	if(type == "yafaray-kdtree-original") accelerator = AcceleratorKdTree::factory(logger, primitives_list, params);
	if(type == "yafaray-kdtree-multi-thread") accelerator = AcceleratorKdTreeMultiThread::factory(logger, primitives_list, params);
	else if(type == "yafaray-simpletest") accelerator = AcceleratorSimpleTest::factory(logger, primitives_list, params);

	if(accelerator) logger.logInfo("Accelerator type '", type, "' created.");
	else
	{
		logger.logWarning("Accelerator type '", type, "' could not be created, using standard single-thread KdTree instead.");
		accelerator = AcceleratorKdTree::factory(logger, primitives_list, params);
	}
	return accelerator;
}

} //namespace yafaray
