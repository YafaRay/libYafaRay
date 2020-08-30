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

#ifndef YAFARAY_IMAGEHANDLER_UTIL_PNG_H
#define YAFARAY_IMAGEHANDLER_UTIL_PNG_H

BEGIN_YAFARAY

#define INV_8  0.00392156862745098039 // 1 / 255
#define INV_16 0.00001525902189669642 // 1 / 65535

class PngDataReader
{
	public:

		PngDataReader(const YByte_t *d, size_t s): size_(s), cursor_(0)
		{
			data_ = new YByte_t[size_];
			for(size_t i = 0; i < size_; i++)
			{
				data_[i] = d[i];
			}
		}

		~PngDataReader()
		{
			delete [] data_;
			data_ = nullptr;
		}

		size_t read(YByte_t *buf, size_t s)
		{
			if(cursor_ > size_) return 0;
			size_t i;

			for(i = 0; i < s && cursor_ < size_; cursor_++, i++)
			{
				buf[i] = data_[cursor_];
			}

			return i;
		}

	private:
		YByte_t *data_;
		size_t size_;
		size_t cursor_;
};

void readFromMem__(png_structp png_ptr, png_bytep buffer, png_size_t bytes_to_read)
{
	PngDataReader *img = (PngDataReader *) png_get_io_ptr(png_ptr);

	if(img == nullptr) png_error(png_ptr, "The image data pointer is null!!");

	if(img->read((YByte_t *)buffer, (size_t)bytes_to_read) < bytes_to_read) png_warning(png_ptr, "EOF Found while reading image data");
}

END_YAFARAY

#endif // YAFARAY_IMAGEHANDLER_UTIL_PNG_H