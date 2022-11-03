#pragma once
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

#ifndef LIBYAFARAY_DYNAMIC_ARRAY_H
#define LIBYAFARAY_DYNAMIC_ARRAY_H

#include <limits>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace yafaray {

template <typename T, typename IndexType>
class DynamicArray final
{
		static_assert(std::is_pod<T>::value, "This class can only be instantiated for simple 'plain old data' types");

	public:
		DynamicArray() = default;
		DynamicArray(const DynamicArray<T, IndexType> &dynamic_array) = default;
		DynamicArray(DynamicArray<T, IndexType> &&dynamic_array) noexcept = default;
		~DynamicArray();
		bool changeCapacity(IndexType capacity);
		bool resize(IndexType size, bool shrink_if_smaller);
		bool pushBack(T item);
		std::pair<T, bool> get(IndexType index) const { if(index >= size_) return {T{0}, false}; else return {items_[index], true}; }
		bool set(IndexType index, T item) const { if(index >= size_) return false; else { items_[index] = item; return true; } }
		void clear(bool shrink);
		IndexType capacity() const { return capacity_; }
		IndexType size() const { return size_; }
		bool empty() const { return size_ == 0; }

	private:
		T *items_{nullptr}; //No need to construct/destroy T when deallocating/shrinking, as it is restricted to be a plain old data type. Same for copying/moving
		IndexType capacity_{0};
		IndexType size_{0};
};

template <typename T, typename IndexType>
inline DynamicArray<T, IndexType>::~DynamicArray()
{
	free(items_);
}

template <typename T, typename IndexType>
inline bool DynamicArray<T, IndexType>::changeCapacity(IndexType capacity)
{
	if(capacity == 0)
	{
		clear(true);
		return true;
	}
	T* items{(T*) realloc (items_, capacity * sizeof(T))};
	if(!items) return false;
	items_ = items;
	capacity_ = capacity;
	if(size_ > capacity_) size_ = capacity_;
	return true;
}

template <typename T, typename IndexType>
inline bool DynamicArray<T, IndexType>::resize(IndexType size, bool shrink_if_smaller)
{
	if(size <= capacity_)
	{
		size_ = size;
		if(shrink_if_smaller) return changeCapacity(size);
		else return true;
	}
	else
	{
		const bool result_capacity_change{changeCapacity(size)};
		if(result_capacity_change)
		{
			size_ = size;
			return true;
		}
		else return false;
	}
}

template <typename T, typename IndexType>
inline void DynamicArray<T, IndexType>::clear(bool shrink)
{
	size_ = 0;
	if(shrink)
	{
		free(items_);
		items_ = nullptr;
		capacity_ = 0;
	}
}

template<typename T, typename IndexType>
bool DynamicArray<T, IndexType>::pushBack(T item)
{
	if(size_ >= capacity_)
	{
		bool capacity_change_result{false};
		if(capacity_ == 0) capacity_change_result = changeCapacity(1);
		else capacity_change_result = changeCapacity(capacity_ * 2);
		if(!capacity_change_result) return false;
	}
	items_[size_] = item;
	++size_;
	return true;
}

} //namespace yafaray

#endif //LIBYAFARAY_DYNAMIC_ARRAY_H
