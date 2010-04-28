/****************************************************************************
 *
 * 		imageOutput.h: generic color output based on imageHandlers
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

#ifndef Y_IMAGE_OUTPUT_H
#define Y_IMAGE_OUTPUT_H

#include <core_api/imagehandler.h>
#include <core_api/output.h>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT imageOutput_t : public colorOutput_t
{
	public:
		imageOutput_t(imageHandler_t *handle, const std::string &name);
		imageOutput_t(); //!< Dummy initializer
		virtual ~imageOutput_t();
		virtual bool putPixel(int x, int y, const float *c, bool alpha = true, bool depth = false, float z = 0.f);
		virtual void flush();
		virtual void flushArea(int x0, int y0, int x1, int y1) {}; // not used by images... yet
	private:
		imageHandler_t *image;
		std::string fname;
};

__END_YAFRAY

#endif // Y_IMAGE_OUTPUT_H

