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

#ifndef YAFARAY_COLLECTION_H
#define YAFARAY_COLLECTION_H

#include "yafaray_conf.h"
#include <map>

BEGIN_YAFARAY

template <typename K, typename T>
class Collection
{
	public:
		size_t size() const { return items_.size(); }
		void set(const K &key, const T &item) { items_[key] = item; };
		void clear() { items_.clear(); };
		typename std::map<K, T>::iterator begin() { return items_.begin(); }
		typename std::map<K, T>::iterator end() { return items_.end(); }
		typename std::map<K, T>::const_iterator begin() const { return items_.begin(); }
		typename std::map<K, T>::const_iterator end() const { return items_.end(); }
		T &operator()(const K &key) { return items_.at(key); }
		const T &operator()(const K &key) const { return items_.at(key); }
		T *find(const K &key);
		const T *find(const K &key) const;

	protected:
		std::map<K, T> items_;
};

template <typename K, typename T>
inline T *Collection<K, T>::find(const K &key)
{
	auto i = items_.find(key);
	if(i != items_.end()) return &i->second;
	else return nullptr;
}

template <typename K, typename T>
inline const T *Collection<K, T>::find(const K &key) const
{
	auto i = items_.find(key);
	if(i != items_.end()) return &i->second;
	else return nullptr;
}

END_YAFARAY

#endif //YAFARAY_COLLECTION_H
