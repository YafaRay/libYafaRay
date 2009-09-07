
#include "png.h"
extern "C"
{
	#include <setjmp.h>
}

#include <utilities/buffer.h>

__BEGIN_YAFRAY

bool isBigEndian()
{
	short int word = 0x0001;
	char *byte = (char *) &word;
	return(byte[0] ? false : true);
}

bool is_png_file(FILE *fp)
{
	unsigned char header[9];
    if(!fp) return false;
    fread(header, 1, 8, fp);
	rewind(fp);
    if(png_sig_cmp(header, 0, 8)) return false;
	return true;
}

gBuf_t<unsigned char, 4> * load_png(const char *name)
{
	FILE *input;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
	gBuf_t<unsigned char, 4> *image=0;
	unsigned char *pixels=0;
	png_bytepp row_pointers = 0;
	
	input = fopen(name,"rb");
	if(input==NULL) {
		std::cout << "File " << name << " not found\n";
		return NULL;
	}
	
	if( !is_png_file(input) ) return(0);
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,	NULL, NULL, NULL);
	if(!png_ptr)
	{
		std::cerr << "png_create_read_struct failed\n";
		return 0;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, (png_infopp)0, (png_infopp)0);
		std::cerr << "png_create_info_struct failed\n";
		return 0;
	}
	
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		if(image) delete image;
		if(pixels) delete[] pixels;
		if(row_pointers) delete[] row_pointers;
		fclose(input);
		return 0;
	}
	
	png_init_io(png_ptr, input);
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
	pixels = new unsigned char[width * height * bytesperpixel];
	row_pointers = new png_bytep[height];
	for(unsigned int i=0; i<height; ++i)
	{
		row_pointers[i] = pixels + i * width * bytesperpixel * sizeof(png_byte);
	}
	png_read_image(png_ptr, row_pointers);
	
	image = new gBuf_t<unsigned char, 4>(width, height);
	unsigned char *to = (*image)(0, 0);
	unsigned char *from = pixels;
	switch(numChan)
	{
		case 4:
			for(unsigned int i=0; i < width*height; ++i) {
				to[0] = from[0];
				to[1] = from[1];
				to[2] = from[2];
				to[3] = from[3];
				to += 4; from += 4;
			}
			break;
		case 3:
			for(unsigned int i=0; i < width*height; ++i) {
				to[0] = from[0];
				to[1] = from[1];
				to[2] = from[2];
				to[3] = 0xff;
				to += 4; from += 3;
			}
			break;
		case 2:
			for(unsigned int i=0; i < width*height; ++i) {
				to[0] = to[1] = to[2] = from[0];
				to[3] = from[1];
				to += 4; from += 2;
			}
			break;
		case 1:
			for(unsigned int i=0; i < width*height; ++i) {
				to[0] = to[1] = to[2] = from[0];
				to[3] = 0xff;
				to += 4; from++;
			}
			break;
	}
	
	png_read_end(png_ptr, info_ptr);
	// cleanup:
	delete[] pixels;
	delete[] row_pointers;
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	fclose(input);
	
	return image;
}

__END_YAFRAY
