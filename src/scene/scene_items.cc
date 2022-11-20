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
#include "geometry/object/object.h"
#include "texture/texture.h"
#include "image/image_output.h"
#include "render/render_view.h"
#include "camera/camera.h"
#include "light/light.h"
#include "volume/region/volume_region.h"

namespace yafaray {

template class SceneItems<Material>;
template class SceneItems<Object>;
template class SceneItems<Light>;
template class SceneItems<Texture>;
template class SceneItems<RenderView>;
template class SceneItems<Camera>;
template class SceneItems<ImageOutput>;
template class SceneItems<VolumeRegion>;
template class SceneItems<Image>;

template <typename T>
std::pair<size_t, ResultFlags> SceneItems<T>::add(const std::string &name, std::unique_ptr<T> item)
{
	if(!item) return {0, YAFARAY_RESULT_ERROR_WHILE_CREATING};
	auto [id, result_flags]{findIdFromName(name)};
	if(result_flags == YAFARAY_RESULT_ERROR_NOT_FOUND)
	{
		id = scene_items_.size();
		scene_items_.emplace_back(SceneItem<T>{std::move(item), name, true});
		result_flags = YAFARAY_RESULT_OK;
	}
	else
	{
		scene_items_[id] = {std::move(item), name, true};
		result_flags = YAFARAY_RESULT_WARNING_OVERWRITTEN;
	}
	scene_items_[id].item_->setId(id);
	names_to_id_[name] = id;
	return {id, result_flags};
}

template <typename T>
ResultFlags SceneItems<T>::rename(size_t id, const std::string &name)
{
	if(id >= scene_items_.size()) return YAFARAY_RESULT_ERROR_NOT_FOUND;
	else
	{
		if(findIdFromName(name).second != YAFARAY_RESULT_ERROR_NOT_FOUND) return YAFARAY_RESULT_ERROR_DUPLICATED_NAME;
		else
		{
			auto map_entry_extracted{names_to_id_.extract(scene_items_[id].name_)};
			map_entry_extracted.key() = name;
			names_to_id_.insert(std::move(map_entry_extracted));
			scene_items_[id].name_ = name;
			return YAFARAY_RESULT_OK;
		}
	}
}

template <typename T>
ResultFlags SceneItems<T>::disable(const std::string &name)
{
	const auto it{names_to_id_.find(name)};
	if(it != names_to_id_.end())
	{
		const auto id{it->second};
		return disable(id);
	}
	else return YAFARAY_RESULT_ERROR_NOT_FOUND;
}

template <typename T>
ResultFlags SceneItems<T>::disable(size_t id)
{
	if(id >= scene_items_.size()) return YAFARAY_RESULT_ERROR_NOT_FOUND;
	else
	{
		scene_items_[id].enabled_ = false;
		return YAFARAY_RESULT_OK;
	}
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
	if(id >= scene_items_.size()) return {{}, YAFARAY_RESULT_ERROR_NOT_FOUND};
	else return {scene_items_[id].name_, YAFARAY_RESULT_OK};
}

template <typename T>
std::pair<T *, ResultFlags> SceneItems<T>::getById(size_t id) const
{
	if(id >= scene_items_.size()) return {nullptr, YAFARAY_RESULT_ERROR_NOT_FOUND};
	else return {scene_items_[id].item_.get(), YAFARAY_RESULT_OK};
}

template<typename T>
std::tuple<T *, size_t, ResultFlags> SceneItems<T>::getByName(const std::string &name) const
{
	auto [id, result]{findIdFromName(name)};
	if(result == YAFARAY_RESULT_ERROR_NOT_FOUND) return {nullptr, id, result};
	auto [ptr, ptr_result]{getById(id)};
	result |= ptr_result;
	return {ptr, id, result};
}

template <typename T>
void SceneItems<T>::clear()
{
	names_to_id_.clear();
	scene_items_.clear();
}

} //namespace yafaray