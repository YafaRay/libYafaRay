#pragma once

#ifndef Y_TILEDARRAY_H
#define Y_TILEDARRAY_H

#include <yafray_constants.h>
#include <cstring>
#include "y_alloc.h"

__BEGIN_YAFRAY

template<class T, int logBlockSize> class tiledArray2D_t
{
public:
	tiledArray2D_t(): data(nullptr), nx(0), ny(0), xBlocks(0)
	{
		blockSize = 1 << logBlockSize;
		blockMask = blockSize-1;
	}
	tiledArray2D_t(int x, int y, bool init=false): nx(x), ny(y)
	{
		blockSize = 1 << logBlockSize;
		blockMask = blockSize-1;
		xBlocks = roundUp(x) >> logBlockSize;
		int nAlloc = roundUp(x) * roundUp(y);
		data = (T *)y_memalign(64, nAlloc * sizeof(T));
		if(init)
		{
			for (int i = 0; i < nAlloc; ++i) new (&data[i]) T();
		}
	}
	void resize(int x, int y, bool init=false)
	{
		xBlocks = roundUp(x) >> logBlockSize;
		int nAlloc = roundUp(x) * roundUp(y);
		T *old = data;
		if(old) y_free(old);
		data = (T *)y_memalign(64, nAlloc * sizeof(T));
		if(init)
		{
			for (int i = 0; i < nAlloc; ++i) new (&data[i]) T();
		}
		nx = x; ny = y;
		xBlocks = roundUp(x) >> logBlockSize;
	}
	int tileSize() const { return blockSize; }
	int xSize() const { return nx; }
	int ySize() const { return ny; }
	T* getData(){ return data; }
	unsigned int size() const { return roundUp(nx) * roundUp(ny); }
	int roundUp(int x) const {
		return (x + blockSize - 1) & ~(blockSize - 1);
	}
	
	~tiledArray2D_t() {
		for (int i = 0; i < nx * ny; ++i)
			data[i].~T();
		if(data) y_free(data);
	}
	T &operator()(int x, int y) {
		int bx = block(x), by = block(y);
		int ox = offset(x), oy = offset(y);
		int offset = (xBlocks * by + bx) << (logBlockSize*2);
		offset += (oy << logBlockSize) + ox;
		return data[offset];
	}
	const T &operator()(int x, int y) const {
		int bx = block(x), by = block(y);
		int ox = offset(x), oy = offset(y);
		int offset = (xBlocks * by + bx) << (logBlockSize*2);
		offset += (oy << logBlockSize) + ox;
		return data[offset];
	}
protected:
	int block(int a) const { return (a >> logBlockSize); }
	int offset(int a) const { return (a & blockMask); }
	// BlockedArray Private Data
	T *data;
	int nx, ny, xBlocks;
	int blockSize, blockMask;
};


template<int logBlockSize> class tiledBitArray2D_t
{
	public:
		tiledBitArray2D_t(int x, int y, bool init=false): nx(x), ny(y)
		{
			blockSize = 1 << logBlockSize;
			blockMask = blockSize-1;
			xBlocks = roundUp(x) >> logBlockSize;
			nAlloc = roundUp(x) * roundUp(y);
			data = (unsigned int *)y_memalign(64, nAlloc * sizeof(unsigned int));
			if(init) std::memset(data, 0, nAlloc);
		}
		~tiledBitArray2D_t() { if(data) y_free(data); }
		int roundUp(int x) const{ return (x + blockSize - 1) & ~(blockSize - 1); }
		void clear(){ std::memset(data, 0, nAlloc); }
		void setBit(int x, int y)
		{
			int bx = block(x), by = block(y);
			int ox = offset(x), oy = offset(y);
			int block_offset = (xBlocks * by + bx) << ((logBlockSize)*2);
			int bit_offset = block_offset + ((oy << logBlockSize) | ox);
			int el_offset = bit_offset >> 5;
			int word_offset = bit_offset & 31;
			data[el_offset] |= (1 << word_offset);
		}
		void clearBit(int x, int y)
		{
			int bx = block(x), by = block(y);
			int ox = offset(x), oy = offset(y);
			int block_offset = (xBlocks * by + bx) << ((logBlockSize)*2);
			int bit_offset = block_offset + ((oy << logBlockSize) | ox);
			int el_offset = bit_offset >> 5;
			int word_offset = bit_offset & 31;
			data[el_offset] &= ~(1 << word_offset);
		}	
		bool getBit(int x, int y)
		{
			int bx = block(x), by = block(y);
			int ox = offset(x), oy = offset(y);
			int block_offset = (xBlocks * by + bx) << ((logBlockSize)*2);
			int bit_offset = block_offset + ((oy << logBlockSize) | ox);
			int el_offset = bit_offset >> 5;
			int word_offset = bit_offset & 31;
			return data[el_offset] & (1 << word_offset);
		}
	protected:
		tiledBitArray2D_t();
		int block(int a) const { return (a >> logBlockSize); }
		int offset(int a) const { return (a & blockMask); }
		// BlockedArray Private Data
		unsigned int *data;
		size_t nAlloc;
		int nx, ny, xBlocks;
		int blockSize, blockMask;
};

__END_YAFRAY
#endif // Y_TILEDARRAY_H
