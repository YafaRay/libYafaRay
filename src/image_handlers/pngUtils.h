/****************************************************************************
 *
 * 		pngUtils.h: Portable Network Graphics format utilities
 *      This is part of the yafray package
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

__BEGIN_YAFRAY

#define inv8  0.00392156862745098039 // 1 / 255
#define inv16 0.00001525902189669642 // 1 / 65535

class pngDataReader_t
{
public:

	pngDataReader_t(const yByte *d, size_t s):size(s), cursor(0)
	{
		data = new yByte[size];
		for(size_t i = 0;i<size;i++)
		{
			data[i] = d[i];
		}
	}

	~pngDataReader_t()
	{
		delete [] data;
		data = nullptr;
	}

	size_t read(yByte* buf, size_t s)
	{
		if(cursor > size) return 0;
		size_t i;

		for(i = 0;i < s && cursor < size;cursor++, i++)
		{
			buf[i] = data[cursor];
		}

		return i;
	}

private:
	yByte *data;
	size_t size;
	size_t cursor;
};

void readFromMem(png_structp pngPtr, png_bytep buffer, png_size_t bytesToRead)
{
   pngDataReader_t *img = (pngDataReader_t*) png_get_io_ptr(pngPtr);

   if(img == nullptr) png_error(pngPtr, "The image data pointer is null!!");

   if(img->read((yByte*)buffer, (size_t)bytesToRead) < bytesToRead) png_warning(pngPtr, "EOF Found while reading image data");
}

__END_YAFRAY
