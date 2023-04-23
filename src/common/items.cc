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

#include "common/items.h"
#include "material/material.h"
#include "geometry/object/object.h"
#include "texture/texture.h"
#include "image/image_output.h"
#include "camera/camera.h"
#include "light/light.h"
#include "volume/region/volume_region.h"
#include "render/imagefilm.h"
#include "render/renderer.h"
#include "scene/scene.h"

namespace yafaray {

template <typename T>
template <typename K>
std::pair<size_t, ParamResult> Items<T>::createItem(Logger &logger, Items <T> &map, const std::string &name, const ParamMap &param_map, const K &items_container)
{

	const auto [existing_item, existing_item_id, existing_item_result]{map.getByName(name)};
	if(existing_item)
	{
		if(logger.isVerbose()) logger.logWarning(items_container.getClassName(), "'", items_container.getName(), "': item with name '", name, "' already exists, overwriting with new item.");
	}
	std::unique_ptr<T> new_item;
	ParamResult param_result;
	if constexpr (std::is_same_v<T, Light> || std::is_same_v<T, Texture> || std::is_same_v<T, Image> || std::is_same_v<T, VolumeRegion> || std::is_same_v<T, Object>) std::tie(new_item, param_result) = T::factory(logger, items_container, name, param_map);
	else std::tie(new_item, param_result) = T::factory(logger, name, param_map);
	if(new_item)
	{
		if(logger.isVerbose())
		{
			logger.logVerbose(items_container.getClassName(), "'", items_container.getName(), "': Added ", new_item->getClassName(), " '", name, "' (", new_item->type().print(), ")!");
		}
		const auto [new_item_id, adding_result]{map.add(name, std::move(new_item))};
		param_result.flags_ |= adding_result;
		return {new_item_id, param_result};
	}
	else return {0, {YAFARAY_RESULT_ERROR_WHILE_CREATING}};
}

template <typename T>
std::pair<size_t, ResultFlags> Items<T>::add(const std::string &name, std::unique_ptr<T> item)
{
	if(!item) return {0, YAFARAY_RESULT_ERROR_WHILE_CREATING};
	auto [id, result_flags]{findIdFromName(name)};
	if(result_flags == YAFARAY_RESULT_ERROR_NOT_FOUND)
	{
		id = items_.size();
		items_.emplace_back(Item<T>{std::move(item), name, true});
		result_flags = YAFARAY_RESULT_OK;
	}
	else
	{
		items_[id] = {std::move(item), name, true};
		result_flags = YAFARAY_RESULT_WARNING_OVERWRITTEN;
	}
	items_[id].item_->setId(id);
	names_to_id_[name] = id;
	modified_items_.insert(id);
	return {id, result_flags};
}

template <typename T>
ResultFlags Items<T>::rename(size_t id, const std::string &name)
{
	if(id >= items_.size()) return YAFARAY_RESULT_ERROR_NOT_FOUND;
	else
	{
		if(findIdFromName(name).second != YAFARAY_RESULT_ERROR_NOT_FOUND) return YAFARAY_RESULT_ERROR_DUPLICATED_NAME;
		else
		{
			auto map_entry_extracted{names_to_id_.extract(items_[id].name_)};
			map_entry_extracted.key() = name;
			names_to_id_.insert(std::move(map_entry_extracted));
			items_[id].name_ = name;
			return YAFARAY_RESULT_OK;
		}
	}
}

template <typename T>
ResultFlags Items<T>::disable(const std::string &name)
{
	if(items_.empty()) return YAFARAY_RESULT_ERROR_NOT_FOUND;
	else if(const auto it{names_to_id_.find(name)}; it != names_to_id_.end())
	{
		const auto id{it->second};
		return disable(id);
	}
	else return YAFARAY_RESULT_ERROR_NOT_FOUND;
}

template <typename T>
ResultFlags Items<T>::disable(size_t id)
{
	if(items_.empty() || id >= items_.size()) return YAFARAY_RESULT_ERROR_NOT_FOUND;
	else
	{
		items_[id].enabled_ = false;
		modified_items_.insert(id);
		return YAFARAY_RESULT_OK;
	}
}

template <typename T>
std::pair<size_t, ResultFlags> Items<T>::findIdFromName(const std::string &name) const
{
	auto it = names_to_id_.find(name);
	if(it != names_to_id_.end()) return {it->second, YAFARAY_RESULT_OK};
	else return {0, YAFARAY_RESULT_ERROR_NOT_FOUND};
}

template <typename T>
std::pair<std::string, ResultFlags> Items<T>::findNameFromId(size_t id) const
{
	if(id >= items_.size()) return {{}, YAFARAY_RESULT_ERROR_NOT_FOUND};
	else return {items_[id].name_, YAFARAY_RESULT_OK};
}

template <typename T>
std::pair<T *, ResultFlags> Items<T>::getById(size_t id) const
{
	if(id >= items_.size()) return {nullptr, YAFARAY_RESULT_ERROR_NOT_FOUND};
	else return {items_[id].item_.get(), YAFARAY_RESULT_OK};
}

template<typename T>
std::tuple<T *, size_t, ResultFlags> Items<T>::getByName(const std::string &name) const
{
	auto [id, result]{findIdFromName(name)};
	if(result == YAFARAY_RESULT_ERROR_NOT_FOUND) return {nullptr, id, result};
	auto [ptr, ptr_result]{getById(id)};
	result |= ptr_result;
	return {ptr, id, result};
}

template <typename T>
void Items<T>::clear()
{
	names_to_id_.clear();
	items_.clear();
	clearModifiedList();
}

//Important: to avoid undefined symbols in CLang, always keep the template explicit specializations **at the end** of the source file!
template class Items<Material>;
template class Items<Object>;
template class Items<Light>;
template class Items<Texture>;
template class Items<Camera>;
template class Items<ImageOutput>;
template class Items<VolumeRegion>;
template class Items<Image>;
template std::pair<size_t, ParamResult> Items<ImageOutput>::createItem<ImageFilm>(Logger &logger, Items<ImageOutput> &map, const std::string &name, const ParamMap &param_map, const ImageFilm &items_container);
template std::pair<size_t, ParamResult> Items<Camera>::createItem<Renderer>(Logger &logger, Items<Camera> &map, const std::string &name, const ParamMap &param_map, const Renderer &items_container);
template std::pair<size_t, ParamResult> Items<Light>::createItem<Scene>(Logger &logger, Items<Light> &map, const std::string &name, const ParamMap &param_map, const Scene &items_container);
template std::pair<size_t, ParamResult> Items<Texture>::createItem<Scene>(Logger &logger, Items<Texture> &map, const std::string &name, const ParamMap &param_map, const Scene &items_container);
template std::pair<size_t, ParamResult> Items<VolumeRegion>::createItem<Scene>(Logger &logger, Items<VolumeRegion> &map, const std::string &name, const ParamMap &param_map, const Scene &items_container);
template std::pair<size_t, ParamResult> Items<Image>::createItem<Scene>(Logger &logger, Items<Image> &map, const std::string &name, const ParamMap &param_map, const Scene &items_container);
template std::pair<size_t, ParamResult> Items<Object>::createItem<Scene>(Logger &logger, Items<Object> &map, const std::string &name, const ParamMap &param_map, const Scene &items_container);

} //namespace yafaray