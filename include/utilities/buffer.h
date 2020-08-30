#pragma once
/****************************************************************************
 *
 *      buffer.h: Buffers (color and float) api
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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

#ifndef YAFARAY_BUFFER_H
#define YAFARAY_BUFFER_H

#include <yafray_constants.h>
BEGIN_YAFRAY

template<typename T1, unsigned char T2>
class /*YAFRAYCORE_EXPORT*/ GBuf final // no need to export this...not instantiated in yafraycore even
{
	public :
		GBuf(int x, int y)
		{
			data_ = new T1 [x * y * T2];
			mx_ = x;
			my_ = y;
		}

		GBuf() { data_ = nullptr; }
		~GBuf() { if(data_) delete[] data_; }

		void set(int x, int y)
		{
			if(data_) delete[] data_;
			data_ = new T1 [x * y * T2];
			mx_ = x;
			my_ = y;
		}

		T1 *operator()(int x, int y) { return &data_[(y * mx_ + x) * T2]; }

		GBuf &operator = (const GBuf &source)
		{
			if((mx_ != source.mx_) || (my_ != source.my_)) std::cerr << "Error, trying to assign buffers of a different size\n";
			if((data_ == nullptr) || (source.data_ == nullptr)) std::cerr << "Assigning unallocated buffers\n";
			int total = mx_ * my_ * T2;
			for(int i = 0; i < total; ++i)
				data_[i] = source.data_[i];
			return *this;
		}

		int resx() const { return mx_; };
		int resy() const { return my_; };

	private:
		T1 *data_ = nullptr;
		int mx_, my_;
};

typedef GBuf<unsigned char, 4> CBuffer_t;
typedef GBuf<float, 4> FcBuffer_t;	//float RGBA buffer

template<class T>
class Buffer final
{
	public :
		Buffer(int x, int y);
		Buffer() { data_ = nullptr;};
		~Buffer();

		void set(int x, int y);
		T &operator()(int x, int y) {return data_[y * mx_ + x];};
		T *buffer(int x, int y) {return &data_[y * mx_ + x];};
		const T &operator()(int x, int y) const {return data_[y * mx_ + x];};
		Buffer<T> &operator = (const Buffer<T> &source);
		int resx() const {return mx_;};
		int resy() const {return my_;};

	private:
		T *data_ = nullptr;
		int mx_, my_;
};

template<class T>
Buffer<T>::Buffer(int x, int y)
{
	data_ = new T [x * y];
	mx_ = x;
	my_ = y;
}

template<class T>
Buffer<T>::~Buffer()
{
	if(data_ != nullptr)
		delete [] data_;
}

template<class T>
void Buffer<T>::set(int x, int y)
{
	if(data_ != nullptr)
		delete [] data_;
	data_ = new T [x * y];
	mx_ = x;
	my_ = y;
}

template<class T>
Buffer<T> &Buffer<T>::operator = (const Buffer<T> &source)
{
	if((mx_ != source.mx_) || (my_ != source.my_))
	{
		std::cout << "Error, trying to assign  buffers of a diferent size\n";
	}
	if((data_ == nullptr) || (source.data_ == nullptr))
	{
		std::cout << "Assigning unallocated buffers\n";
	}
	int total = mx_ * my_;
	for(int i = 0; i < total; ++i) data_[i] = source.data_[i];

	return *this;

}

END_YAFRAY

#endif
