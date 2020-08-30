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

#ifndef YAFARAY_UTIL_TILED_ARRAY_H
#define YAFARAY_UTIL_TILED_ARRAY_H

#include "constants.h"
#include <cstring>
#include "util_aligned_alloc.h"

BEGIN_YAFARAY

template<class T, int logBlockSize> class TiledArray2D
{
	public:
		TiledArray2D(): data_(nullptr), nx_(0), ny_(0), x_blocks_(0)
		{
			block_size_ = 1 << logBlockSize;
			block_mask_ = block_size_ - 1;
		}
		TiledArray2D(int x, int y, bool init = false): nx_(x), ny_(y)
		{
			block_size_ = 1 << logBlockSize;
			block_mask_ = block_size_ - 1;
			x_blocks_ = roundUp(x) >> logBlockSize;
			int n_alloc = roundUp(x) * roundUp(y);
			data_ = (T *) yMemalign__(64, n_alloc * sizeof(T));
			if(init)
			{
				for(int i = 0; i < n_alloc; ++i) new(&data_[i]) T();
			}
		}
		void resize(int x, int y, bool init = false)
		{
			x_blocks_ = roundUp(x) >> logBlockSize;
			int n_alloc = roundUp(x) * roundUp(y);
			T *old = data_;
			if(old) yFree__(old);
			data_ = (T *) yMemalign__(64, n_alloc * sizeof(T));
			if(init)
			{
				for(int i = 0; i < n_alloc; ++i) new(&data_[i]) T();
			}
			nx_ = x; ny_ = y;
			x_blocks_ = roundUp(x) >> logBlockSize;
		}
		int tileSize() const { return block_size_; }
		int xSize() const { return nx_; }
		int ySize() const { return ny_; }
		T *getData() { return data_; }
		unsigned int size() const { return roundUp(nx_) * roundUp(ny_); }
		int roundUp(int x) const
		{
			return (x + block_size_ - 1) & ~(block_size_ - 1);
		}

		~TiledArray2D()
		{
			for(int i = 0; i < nx_ * ny_; ++i)
				data_[i].~t_();
			if(data_) yFree__(data_);
		}
		T &operator()(int x, int y)
		{
			int bx = block(x), by = block(y);
			int ox = offset(x), oy = offset(y);
			int offset = (x_blocks_ * by + bx) << (logBlockSize * 2);
			offset += (oy << logBlockSize) + ox;
			return data_[offset];
		}
		const T &operator()(int x, int y) const
		{
			int bx = block(x), by = block(y);
			int ox = offset(x), oy = offset(y);
			int offset = (x_blocks_ * by + bx) << (logBlockSize * 2);
			offset += (oy << logBlockSize) + ox;
			return data_[offset];
		}
	protected:
		int block(int a) const { return (a >> logBlockSize); }
		int offset(int a) const { return (a & block_mask_); }
		// BlockedArray Private Data
		T *data_;
		int nx_, ny_, x_blocks_;
		int block_size_, block_mask_;
};


template<int logBlockSize> class TiledBitArray2D
{
	public:
		TiledBitArray2D(int x, int y, bool init = false): nx_(x), ny_(y)
		{
			block_size_ = 1 << logBlockSize;
			block_mask_ = block_size_ - 1;
			x_blocks_ = roundUp(x) >> logBlockSize;
			n_alloc_ = roundUp(x) * roundUp(y);
			data_ = (unsigned int *) yMemalign__(64, n_alloc_ * sizeof(unsigned int));
			if(init) std::memset(data_, 0, n_alloc_);
		}
		~TiledBitArray2D() { if(data_) yFree__(data_); }
		int roundUp(int x) const { return (x + block_size_ - 1) & ~(block_size_ - 1); }
		void clear() { std::memset(data_, 0, n_alloc_); }
		void setBit(int x, int y)
		{
			int bx = block(x), by = block(y);
			int ox = offset(x), oy = offset(y);
			int block_offset = (x_blocks_ * by + bx) << ((logBlockSize) * 2);
			int bit_offset = block_offset + ((oy << logBlockSize) | ox);
			int el_offset = bit_offset >> 5;
			int word_offset = bit_offset & 31;
			data_[el_offset] |= (1 << word_offset);
		}
		void clearBit(int x, int y)
		{
			int bx = block(x), by = block(y);
			int ox = offset(x), oy = offset(y);
			int block_offset = (x_blocks_ * by + bx) << ((logBlockSize) * 2);
			int bit_offset = block_offset + ((oy << logBlockSize) | ox);
			int el_offset = bit_offset >> 5;
			int word_offset = bit_offset & 31;
			data_[el_offset] &= ~(1 << word_offset);
		}
		bool getBit(int x, int y)
		{
			int bx = block(x), by = block(y);
			int ox = offset(x), oy = offset(y);
			int block_offset = (x_blocks_ * by + bx) << ((logBlockSize) * 2);
			int bit_offset = block_offset + ((oy << logBlockSize) | ox);
			int el_offset = bit_offset >> 5;
			int word_offset = bit_offset & 31;
			return data_[el_offset] & (1 << word_offset);
		}
	protected:
		TiledBitArray2D();
		int block(int a) const { return (a >> logBlockSize); }
		int offset(int a) const { return (a & block_mask_); }
		// BlockedArray Private Data
		unsigned int *data_;
		size_t n_alloc_;
		int nx_, ny_, x_blocks_;
		int block_size_, block_mask_;
};

END_YAFARAY
#endif // YAFARAY_UTIL_TILED_ARRAY_H
