
/****************************************************************************
 *
 * 		imageOutput.cc: generic color output based on imageHandlers
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

#include <yafraycore/imageOutput.h>

__BEGIN_YAFRAY

imageOutput_t::imageOutput_t(imageHandler_t * handle, const std::string &name) : image(handle), fname(name)
{
	//empty
}

imageOutput_t::imageOutput_t()
{
	image = NULL;
}

imageOutput_t::~imageOutput_t()
{
	//empty
}

bool imageOutput_t::putPixel(int x, int y, const float *c, bool alpha, bool depth, float z)
{
	if(image)
	{
		colorA_t col(0.f);
		col.set(c[0], c[1], c[2], ( (alpha) ? c[3] : 1.f ) );
		image->putPixel(x , y, col, z);
	}
	return true;
}

void imageOutput_t::flush()
{
	if(image)
	{
		image->saveToFile(fname);
	}
}

__END_YAFRAY

