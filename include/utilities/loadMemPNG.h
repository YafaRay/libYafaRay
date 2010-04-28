/*
 * loadMemPNG.h - A simple helper to read PNG's embedded in C form
 *
 *  Created on: 26/06/2009
 *
 *      Author: Rodrigo Placencia (DarkTide)
 *		
 *		Based on png.cc from basictex plugin
 */
 
#ifndef LOADMEMPNG_H
#define LOADMEMPNG_H

#include <png.h>

extern "C"
{
	#include <setjmp.h>
}

#include <utilities/buffer.h>
#include <core_api/color.h>

__BEGIN_YAFRAY

typedef unsigned char yafByte;
typedef unsigned int yafUint;
typedef Buffer_t<colorA_t> imgBuffer_t;

#define toFloat(c) ((float)c / 255.f)
#define toFloat16(c) ((float)c / 255.f)

class ImgDataReader
{
public:
	ImgDataReader(const yafByte *d, size_t s):size(s), cursor(0)
	{
		data = new yafByte[size];
		for(size_t i = 0;i<size;i++)
		{
			data[i] = d[i];
		}
	}
	
	~ImgDataReader()
	{
		delete [] data;
		data = NULL;
	}

	size_t read(yafByte* buf, size_t s)
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
	yafByte *data;
	size_t size;
	size_t cursor;
};

void readPngFromMem(png_structp png_ptr, png_bytep buffer, png_size_t bytesToRead)
{
   if(png_ptr->io_ptr == NULL) png_error(png_ptr, "io_ptr is NULL!!");

   ImgDataReader *img = (ImgDataReader*)png_ptr->io_ptr;
   
   if(img->read((yafByte*)buffer, (size_t)bytesToRead) < bytesToRead) png_warning(png_ptr, "EOF Found");
}

bool is_png_data(ImgDataReader *img)
{
	yafByte pngSignature[9];
    
    if(img->read(pngSignature, 8) < 8) return false;
    
    if(png_check_sig(pngSignature, 8)) return true;
	return false;
}

imgBuffer_t * load_mem_png(const yafByte *data, size_t size)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
	imgBuffer_t *image = NULL;
	yafByte *pixels = NULL;
	png_bytepp row_pointers = NULL;
	ImgDataReader *reader = new ImgDataReader(data, size);
	
	if( !is_png_data(reader) ) return 0;
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,	NULL, NULL, NULL);
	if(!png_ptr)
	{
		std::cerr << "png_create_read_struct failed\n";
		delete reader;
		return 0;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, (png_infopp)0, (png_infopp)0);
		std::cerr << "png_create_info_struct failed\n";
		delete reader;
		return 0;
	}
	
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		if(image) delete image;
		if(pixels) delete[] pixels;
		if(row_pointers) delete[] row_pointers;
		delete reader;
		return 0;
	}
	
	png_set_read_fn(png_ptr, (void*)reader, readPngFromMem);

	png_set_sig_bytes(png_ptr, 8);
	
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
	int numChan = png_get_channels(png_ptr, info_ptr);
	int bytesperpixel = (bit_depth == 16) ? 2*numChan : numChan;
	
	switch(color_type)
	{
		case PNG_COLOR_TYPE_RGB:
		case PNG_COLOR_TYPE_RGB_ALPHA:
			break;
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb(png_ptr);
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
				numChan = 4;
			} else {
				numChan = 3;
			}
			break;
		case PNG_COLOR_TYPE_GRAY:
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			if (bit_depth < 8) {
				png_set_expand(png_ptr);
				bit_depth = 8;
			}
			break;
		default:
			std::cout << "PNG format not supported\n";
			longjmp(png_jmpbuf(png_ptr), 1);
	}
	pixels = new yafByte[width * height * bytesperpixel];
	row_pointers = new png_bytep[height];
	for(yafUint i=0; i<height; ++i)
	{
		row_pointers[i] = pixels + i * width * bytesperpixel * sizeof(yafByte);
	}
	png_read_image(png_ptr, row_pointers);
	
	image = new imgBuffer_t(width, height);
	colorA_t *to = image->buffer(0, 0);
	yafByte *from = pixels;
	switch(numChan)
	{
		case 4:
			for(yafUint i=0; i < width*height; ++i) {
				*to = colorA_t(
				toFloat(from[0]),
				toFloat(from[1]),
				toFloat(from[2]),
				toFloat(from[3])
				);
				to++; from += 4;
			}
			break;
		case 3:
			for(yafUint i=0; i < width*height; ++i) {
				*to = colorA_t(
				toFloat(from[0]),
				toFloat(from[1]),
				toFloat(from[2]),
				toFloat(0xFF)
				);
				to++; from += 3;
			}
			break;
		case 2:
			for(yafUint i=0; i < width*height; ++i) {
				float c = toFloat(from[0]);
				*to = colorA_t(
				c,
				c,
				c,
				toFloat(from[1])
				);
				to++; from += 2;
			}
			break;
		case 1:
			for(yafUint i=0; i < width*height; ++i) {
				float c = toFloat(from[0]);
				*to = colorA_t(
				c,
				c,
				c,
				toFloat(0xFF)
				);
				to++; from++;
			}
			break;
	}
	
	png_read_end(png_ptr, info_ptr);
	// cleanup:
	delete[] pixels;
	delete[] row_pointers;
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	delete reader;
	return image;
}

__END_YAFRAY

#endif
