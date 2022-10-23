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

#include "scene/scene_items.h"
#include "material/material.h"

namespace yafaray {

template class SceneItems<Material>;

template <typename T>
std::pair<size_t, ResultFlags> SceneItems<T>::add(const std::string &name, std::unique_ptr<T> item)
{
	if(!item) return {0, YAFARAY_RESULT_ERROR_WHILE_CREATING};
	auto [id, result_flags]{findIdFromName(name)};
	if(result_flags == YAFARAY_RESULT_ERROR_NOT_FOUND)
	{
		id = items_.size();
		items_.emplace_back(std::move(item));
		names_.emplace_back(name);
		result_flags = YAFARAY_RESULT_OK;
	}
	else
	{
		items_[id] = std::move(item);
		names_[id] = name;
		result_flags = YAFARAY_RESULT_WARNING_OVERWRITTEN;
	}
	items_[id]->setId(id);
	names_to_id_[name] = id;
	return {id, result_flags};
}

template <typename T>
ResultFlags SceneItems<T>::rename(size_t id, const std::string &name)
{
	if(id >= items_.size()) return YAFARAY_RESULT_ERROR_NOT_FOUND;
	else
	{
		if(findIdFromName(name).second == YAFARAY_RESULT_ERROR_NOT_FOUND)
		{
			auto map_entry_extracted{names_to_id_.extract(names_[id])};
			map_entry_extracted.key() = name;
			names_to_id_.insert(std::move(map_entry_extracted));
			names_[id] = name;
			return YAFARAY_RESULT_OK;
		}
		else
		{
			return YAFARAY_RESULT_ERROR_DUPLICATED_NAME;
		}
	}
}

template <typename T>
ResultFlags SceneItems<T>::disable(const std::string &name)
{
	auto it = names_to_id_.find(name);
	if(it != names_to_id_.end())
	{
		names_to_id_.erase(it);
		return YAFARAY_RESULT_OK;
	}
	else return YAFARAY_RESULT_ERROR_NOT_FOUND;
}

template <typename T>
ResultFlags SceneItems<T>::disable(size_t id)
{
	if(id >= items_.size()) return YAFARAY_RESULT_ERROR_NOT_FOUND;
	else return disable(names_[id]);
}

template <typename T>
std::pair<size_t, ResultFlags> SceneItems<T>::findIdFromName(const std::string &name) const
{
	auto it = names_to_id_.find(name);
	if(it != names_to_id_.end()) return {it->second, YAFARAY_RESULT_OK};
	else return {0, YAFARAY_RESULT_ERROR_NOT_FOUND};
}

template <typename T>
std::pair<std::string, ResultFlags> SceneItems<T>::findNameFromId(size_t id) const
{
	if(id >= items_.size()) return {{}, YAFARAY_RESULT_ERROR_NOT_FOUND};
	else return {names_[id], YAFARAY_RESULT_OK};
}

template <typename T>
std::pair<T *, ResultFlags> SceneItems<T>::getById(size_t id) const
{
	if(id >= items_.size()) return {nullptr, YAFARAY_RESULT_ERROR_NOT_FOUND};
	else return {items_[id].get(), YAFARAY_RESULT_OK};
}

template <typename T>
void SceneItems<T>::clear()
{
	names_to_id_.clear();
	names_.clear();
	items_.clear();
}

} //namespace yafaray