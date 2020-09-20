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
#include "accelerator/accelerator_kdtree.h"
#include "common/logging.h"
#include "common/param.h"

BEGIN_YAFARAY

class Triangle;
class Primitive;

template class Accelerator<Triangle>;
template class Accelerator<Primitive>;

template<class T>
Accelerator<T> *Accelerator<T>::factory(const T **primitives_list, ParamMap &params)
{
	std::string type;
	params.getParam("type", type);
	if(type == "kdtree")
	{
		Y_INFO << "Accelerator type '" << type << "' created." << YENDL;
		return AcceleratorKdTree<T>::factory(primitives_list, params);
	}
	else
	{
		Y_ERROR << "Accelerator type '" << type << "' could not be created." << YENDL;
		return nullptr;
	}
}

END_YAFARAY
