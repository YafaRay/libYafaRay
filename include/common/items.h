#pragma once
/****************************************************************************
 *
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
 *
 */

#ifndef LIBYAFARAY_ITEMS_H
#define LIBYAFARAY_ITEMS_H

#include "result_flags.h"
#include <set>
#include <memory>

namespace yafaray {

class Logger;
class ParamMap;
struct ParamResult;

template <typename T>
struct Item final
{
	std::unique_ptr<T> item_;
	std::string name_;
	bool enabled_{true};
};

template <typename T>
class Items final
{
	public:
		template <typename K> [[nodiscard]] static std::pair<size_t, ParamResult> createItem(Logger &logger, Items <T> &map, const std::string &name, const ParamMap &param_map, const K &items_container);
		[[nodiscard]] std::pair<size_t, ResultFlags> add(const std::string &name, std::unique_ptr<T> item); //!< Add a unique_ptr to the list of items. Requires T to have a method "T::setId" to modify the Id in the item itself according to the Id determined during the addition
		[[nodiscard]] ResultFlags rename(size_t id, const std::string &name);
		[[nodiscard]] ResultFlags disable(const std::string &name);
		[[nodiscard]] ResultFlags disable(size_t id);
		[[nodiscard]] std::pair<size_t, ResultFlags> findIdFromName(const std::string &name) const;
		[[nodiscard]] std::pair<std::string, ResultFlags> findNameFromId(size_t id) const;
		[[nodiscard]] std::pair<T *, ResultFlags> getById(size_t id) const;
		[[nodiscard]] std::tuple<T *, size_t, ResultFlags> getByName(const std::string &name) const;
		[[nodiscard]] size_t size() const { return items_.size(); }
		[[nodiscard]] bool empty() const { return items_.empty(); }
		[[nodiscard]] bool modified() const { return !modified_items_.empty(); }
		[[nodiscard]] const std::set<size_t> &modifiedList() const { return modified_items_; }
		typename std::vector<Item<T>>::iterator begin() { return items_.begin(); }
		typename std::vector<Item<T>>::iterator end() { return items_.end(); }
		typename std::vector<Item<T>>::const_iterator begin() const { return items_.begin(); }
		typename std::vector<Item<T>>::const_iterator end() const { return items_.end(); }
		void clearModifiedList() { modified_items_.clear(); }
		void clear();

	private:
		std::vector<Item<T>> items_;
		std::map<std::string, size_t> names_to_id_;
		std::set<size_t> modified_items_;
};

} //namespace yafaray

#endif //LIBYAFARAY_ITEMS_H
