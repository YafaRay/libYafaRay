/****************************************************************************
 *
 * 			jpeg.cc: JPEG Texture implementation
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alejandro Conty Estevez
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

#include <yafray_config.h>

#ifdef HAVE_JPEG

#include <iostream>
#include <cstdio>
#include <cstdlib>
using namespace std;

extern "C"
{
#include <setjmp.h>
#include <jpeglib.h>
}
#include <utilities/buffer.h>


__BEGIN_YAFRAY
//****************************************************************************

// error handling for JPEGS,
// this is done so we don't exit even in case of error

METHODDEF(void) _jpeg_errmsg(j_common_ptr cinfo)
{
#ifdef __DEBUG__
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message)(cinfo, buffer);
	cout << "JPEG Library Error: " << buffer << endl;
#endif
}

struct my_jpeg_error_mgr {
	struct jpeg_error_mgr pub;	// public fields
	jmp_buf setjmp_buffer;	// for return to caller
};

// JPEG error handler
typedef struct my_jpeg_error_mgr *error_ptr;

void my_jpeg_error_exit(j_common_ptr cinfo)
{
	error_ptr myerr = (error_ptr)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

//****************************************************************************

gBuf_t<unsigned char, 4> * load_jpeg(const char *name)
{
	struct jpeg_decompress_struct cinfo;
	struct my_jpeg_error_mgr jerr;
	FILE *input;
	gBuf_t<unsigned char, 4> *image;
	unsigned char *p;
	unsigned char **arr;

	input = fopen(name,"rb");
	if (input==NULL) {
		cout << "File " << name << " not found\n";
		return NULL;
	}
	//cout << "Loading jpeg file from " << name << "...\n";

	cinfo.err = jpeg_std_error(&jerr.pub);
	cinfo.err->output_message = _jpeg_errmsg;
	jerr.pub.error_exit = my_jpeg_error_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(input);
		return NULL;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, input);
	jpeg_read_header(&cinfo, TRUE);

	jpeg_start_decompress(&cinfo);

	bool is_gray = ((cinfo.out_color_space==JCS_GRAYSCALE) & (cinfo.output_components==1));
	bool is_rgb = ((cinfo.out_color_space==JCS_RGB) & (cinfo.output_components==3));
	// not read as cmyk, but code to do conversion is there if needed, see below
	bool is_cmyk = ((cinfo.out_color_space==JCS_CMYK) & (cinfo.output_components==4));

#ifdef __DEBUG__
	cout << "Loading image " << cinfo.output_width << " x " << cinfo.output_height << endl;
	cout << "color depth is " << cinfo.output_components*8 << " bits.\n";
#endif

	if ((!is_gray) && (!is_rgb) && (!is_cmyk)) {
		cout << "Unsupported color space: " << cinfo.out_color_space
				 << " depth: " << cinfo.output_components << endl;
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return NULL;
	}

	image = new gBuf_t<unsigned char, 4>(cinfo.output_width, cinfo.output_height);
	if (image==NULL) //uhm...this cannot happen anyway, can it? new does throw exceptions i thought...
	{
		cout << "Error allocating memory\n";
		longjmp(jerr.setjmp_buffer, 2);
	}

	p = (*image)(0,0);

	//colBuf now has alpha component, so have to allocate temporary scanline buffer for color jpeg as well
	unsigned char* gbuf=NULL;
	if (is_gray)
		gbuf = new unsigned char[cinfo.image_width];
	else if (is_rgb)
		gbuf = new unsigned char[cinfo.image_width*3];
	else
		gbuf = new unsigned char[cinfo.image_width*4];
	if (gbuf==NULL) {
		cout << "Error allocating memory for temporary scanline buffer\n";
		return 0;
	}

	while (cinfo.output_scanline < cinfo.output_height) {
		arr = &gbuf;
		jpeg_read_scanlines(&cinfo, arr, 1);
		if (is_gray) {
			for (unsigned int x=0;x<cinfo.image_width;x++) {
				*p++ = gbuf[x];
				*p++ = gbuf[x];
				*p++ = gbuf[x];
				*p++ = 255;
			}
		}
		else if (is_rgb) {
			for (unsigned int x=0;x<cinfo.image_width*3;x+=3) {
				*p++ = gbuf[x];
				*p++ = gbuf[x+1];
				*p++ = gbuf[x+2];
				*p++ = 255;
			}
		}
		else {
			// assume blender nonstandard jpeg with alpha (!!!)
			for (unsigned int x=0;x<cinfo.image_width*4;x+=4) {
				// equivalent of blender code
				p[3] = gbuf[x+3];
				unsigned char nk = 255-p[3];
				p[0] = max(0, min(gbuf[x]-nk, 255));
				p[1] = max(0, min(gbuf[x+1]-nk, 255));
				p[2] = max(0, min(gbuf[x+2]-nk, 255));
				p += 4;

			}
		}
	}
	delete[] gbuf;

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(input);
	return image;
}

__END_YAFRAY

#endif
