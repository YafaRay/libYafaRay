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

#ifndef YAFARAY_MEMORY_ARENA_H
#define YAFARAY_MEMORY_ARENA_H

#include "yafaray_common.h"
#include <vector>
#include <algorithm>
#include <cstdint>

BEGIN_YAFARAY

class MemoryArena
{
	public:
		// MemoryArena Public Methods
		explicit MemoryArena(uint32_t block_size = 32768)
		{
			block_size_ = block_size;
			cur_block_pos_ = 0;
			current_block_ = (char *) malloc(block_size_);
		}
		~MemoryArena()
		{
			free(current_block_);
			for(const auto &used_block : used_blocks_) free(used_block);
			for(const auto &available_block : available_blocks_) free(available_block);
		}
		void *alloc(uint32_t sz)
		{
			// Round up _sz_ to minimum machine alignment
			sz = ((sz + 7) & (~7));
			if(cur_block_pos_ + sz > block_size_)
			{
				// Get new block of memory for _MemoryArena_
				used_blocks_.emplace_back(current_block_);
				if(!available_blocks_.empty() && sz <= block_size_)
				{
					current_block_ = available_blocks_.back();
					available_blocks_.pop_back();
				}
				else
					current_block_ = (char *) malloc((std::max(sz, block_size_)));
				cur_block_pos_ = 0;
			}
			void *ret = current_block_ + cur_block_pos_;
			cur_block_pos_ += sz;
			return ret;
		}
	private:
		// MemoryArena Private Data
		alignas(8) uint32_t cur_block_pos_, block_size_;
		char *current_block_;
		alignas(8) std::vector<char *> used_blocks_, available_blocks_;
};

END_YAFARAY

#endif // YAFARAY_MEMORY_ARENA_H
