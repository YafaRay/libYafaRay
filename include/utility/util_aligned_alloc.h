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

#ifndef YAFARAY_UTIL_ALIGNED_ALLOC_H
#define YAFARAY_UTIL_ALIGNED_ALLOC_H

#include "constants.h"
#include <vector>
#include <algorithm>
#include <cstdint>

#if defined(__FreeBSD__)
#include <stdlib.h>
#elif !defined(WIN32) && !defined(__APPLE__)
#include <alloca.h>
#elif defined (__MINGW32__) //Added by DarkTide to enable mingw32 compliation
#include <malloc.h>
#endif

BEGIN_YAFARAY

#if defined(_WIN32) && !defined(__MINGW32__) //Added by DarkTide to enable mingw32 compliation
#define alloca _alloca
#endif

inline void *yMemalign__(size_t bound, size_t size)
{
#if (defined (_WIN32) && (defined (_MSC_VER)) || defined (__MINGW32__)) //Added by DarkTide to enable mingw32 compliation
	return _aligned_malloc(size, bound);
#elif defined(__APPLE__)
	// there's no good aligned allocation on OS X it seems :/
	// however, malloc is supposed to return at least SSE2 compatible alignment, which has to be enough.
	// alternative would be valloc, but i have no good info on its effects.
	return malloc(size);
	//#elif defined(__FreeBSD__)
#else
	// someone check if this maybe is available on OS X meanwhile...thx :)
	void *ret;
	if(posix_memalign(&ret, bound, size) != 0)
		return (nullptr);
	else
		return (ret);
	//#else
	//	return memalign(bound, size);
#endif
}

inline void yFree__(void *ptr)
{
#if (defined (_WIN32) && (defined (_MSC_VER)) || defined (__MINGW32__)) //Added by DarkTide to enable mingw32 compliation
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}


template <class T> class ObjectArena
{
	public:
		// ObjectArena Public Methods
		ObjectArena()
		{
			n_available_ = 0;
		}
		T *alloc()
		{
			if(n_available_ == 0)
			{
				int n_alloc = std::max((unsigned long)16,
									   (unsigned long)(65536 / sizeof(T)));
				mem_ = (T *) yMemalign__(64, n_alloc * sizeof(T));
				n_available_ = n_alloc;
				to_delete_.push_back(mem_);
			}
			--n_available_;
			return mem_++;
		}
		operator T *()
		{
			return alloc();
		}
		~ObjectArena() { freeAll(); }
		void freeAll()
		{
			for(unsigned int i = 0; i < to_delete_.size(); ++i)
				yFree__(to_delete_[i]);
			to_delete_.erase(to_delete_.begin(), to_delete_.end());
			n_available_ = 0;
		}
	private:
		// ObjectArena Private Data
		T *mem_;
		int n_available_;
		std::vector<T *> to_delete_;
};

class MemoryArena
{
	public:
		// MemoryArena Public Methods
		MemoryArena(uint32_t bs = 32768)
		{
			block_size_ = bs;
			cur_block_pos_ = 0;
			current_block_ = (char *) yMemalign__(64, block_size_);
		}
		~MemoryArena()
		{
			yFree__(current_block_);
			for(uint32_t i = 0; i < used_blocks_.size(); ++i)
				yFree__(used_blocks_[i]);
			for(uint32_t i = 0; i < available_blocks_.size(); ++i)
				yFree__(available_blocks_[i]);
		}
		void *alloc(uint32_t sz)
		{
			// Round up _sz_ to minimum machine alignment
			sz = ((sz + 7) & (~7));
			if(cur_block_pos_ + sz > block_size_)
			{
				// Get new block of memory for _MemoryArena_
				used_blocks_.push_back(current_block_);
				if(available_blocks_.size() && sz <= block_size_)
				{
					current_block_ = available_blocks_.back();
					available_blocks_.pop_back();
				}
				else
					current_block_ = (char *) yMemalign__(64, (std::max(sz, block_size_)));
				cur_block_pos_ = 0;
			}
			void *ret = current_block_ + cur_block_pos_;
			cur_block_pos_ += sz;
			return ret;
		}
		void freeAll()
		{
			cur_block_pos_ = 0;
			while(used_blocks_.size())
			{
				available_blocks_.push_back(used_blocks_.back());
				used_blocks_.pop_back();
			}
		}
	private:
		// MemoryArena Private Data
		uint32_t cur_block_pos_, block_size_;
		char *current_block_;
		std::vector<char *> used_blocks_, available_blocks_;
};

END_YAFARAY
#endif // YAFARAY_UTIL_ALIGNED_ALLOC_H
