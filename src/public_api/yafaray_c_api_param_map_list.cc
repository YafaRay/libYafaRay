/****************************************************************************
 *   This is part of the libYafaRay package
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "public_api/yafaray_c_api.h"
#include "param/param.h"
#include <list>

yafaray_ParamMapList *yafaray_createParamMapList()
{
	auto param_map_list{new std::list<yafaray::ParamMap>()};
	return reinterpret_cast<yafaray_ParamMapList *>(param_map_list);
}

void yafaray_addParamMapToList(yafaray_ParamMapList *param_map_list, const yafaray_ParamMap *param_map)
{
	if(!param_map_list || !param_map) return;
	reinterpret_cast<std::list<yafaray::ParamMap> *>(param_map_list)->emplace_back(*reinterpret_cast<const yafaray::ParamMap *>(param_map));
}

void yafaray_clearParamMapList(yafaray_ParamMapList *param_map_list)
{
	if(!param_map_list) return;
	reinterpret_cast<std::list<yafaray::ParamMap> *>(param_map_list)->clear();
}

void yafaray_destroyParamMapList(yafaray_ParamMapList *param_map_list)
{
	delete reinterpret_cast<std::list<yafaray::ParamMap> *>(param_map_list);
}
