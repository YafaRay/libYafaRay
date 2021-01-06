#pragma once
/****************************************************************************
 *
 *      pngUtils.h: Portable Network Graphics format utilities
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
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

#ifndef YAFARAY_FORMAT_PNG_UTIL_H
#define YAFARAY_FORMAT_PNG_UTIL_H

BEGIN_YAFARAY

class PngDataReader
{
	public:
		PngDataReader(const uint8_t *d, size_t s);
		~PngDataReader();
		size_t read(uint8_t *buf, size_t s);

	private:
		uint8_t *data_;
		size_t size_;
		size_t cursor_ = 0;
};

inline PngDataReader::PngDataReader(const uint8_t *d, size_t s) : size_(s)
{
	data_ = new uint8_t[size_];
	for(size_t i = 0; i < size_; i++) data_[i] = d[i];
}

inline PngDataReader::~PngDataReader()
{
	delete[] data_;
}

inline size_t PngDataReader::read(uint8_t *buf, size_t s)
{
	if(cursor_ > size_) return 0;
	size_t i = 0;
	for( ; i < s && cursor_ < size_; cursor_++, i++) buf[i] = data_[cursor_];
	return i;
}

void readFromMem_global(png_structp png_ptr, png_bytep buffer, png_size_t bytes_to_read)
{
	PngDataReader *img = (PngDataReader *) png_get_io_ptr(png_ptr);
	if(!img) png_error(png_ptr, "The image data pointer is null!!");
	else if(img->read((uint8_t *)buffer, static_cast<size_t>(bytes_to_read)) < bytes_to_read) png_warning(png_ptr, "EOF Found while reading image data");
}

END_YAFARAY

#endif // YAFARAY_FORMAT_PNG_UTIL_H