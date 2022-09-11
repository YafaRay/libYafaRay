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

#ifndef LIBYAFARAY_BIMAP_H
#define LIBYAFARAY_BIMAP_H

#include <map>

namespace yafaray {

template <typename Key1, typename Key2, typename Value>
class BiMap
{
	public:
		BiMap(const std::initializer_list<std::tuple<Key1, Key2, Value>> &initializer_list);
		[[nodiscard]] const std::pair<Key2, Value> *find(const Key1 &key) const;
		[[nodiscard]] const std::pair<Key1, Value> *find(const Key2 &key) const;
		[[nodiscard]] typename std::map<Key1, std::pair<Key2, Value>>::const_iterator begin() const { return map_1_.begin(); }
		[[nodiscard]] typename std::map<Key1, std::pair<Key2, Value>>::const_iterator end() const { return map_1_.end(); }

	private:
		std::map<Key1, std::pair<Key2, Value>> map_1_;
		std::map<Key2, std::pair<Key1, Value>> map_2_;
};

template <typename Key1, typename Key2, typename Value>
inline BiMap<Key1, Key2, Value>::BiMap(const std::initializer_list<std::tuple<Key1, Key2, Value>> &initializer_list)
{
	for(const auto &[key1, key2, value] : initializer_list)
	{
		map_1_[key1] = std::pair<Key2, Value>{key2, value};
		map_2_[key2] = std::pair<Key1, Value>{key1, value};
	}
}

template <typename Key1, typename Key2, typename Value>
inline const std::pair<Key2, Value> *BiMap<Key1, Key2, Value>::find(const Key1 &key) const
{
	const auto &i{map_1_.find(key)};
	if(i != map_1_.end()) return &i->second;
	else return nullptr;
}

template <typename Key1, typename Key2, typename Value>
inline const std::pair<Key1, Value> *BiMap<Key1, Key2, Value>::find(const Key2 &key) const
{
	const auto &i{map_2_.find(key)};
	if(i != map_2_.end()) return &i->second;
	else return nullptr;
}

} //namespace yafaray

#endif //LIBYAFARAY_BIMAP_H
