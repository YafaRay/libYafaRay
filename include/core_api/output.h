/****************************************************************************
 *
 * 		output.h: Output base class
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Est�vez
 *		Modifyed by Rodrigo Placencia Vazquez (2009)
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
#ifndef Y_COUTPUT_H
#define Y_COUTPUT_H

#include "yafray_constants.h"

__BEGIN_YAFRAY

/*! Base class for rendering output containers */

class colorOutput_t
{
	public:
		virtual ~colorOutput_t() {};
		virtual bool putPixel(int x, int y, const float *c, bool alpha = true, bool depth = false, float z = 0.f)=0;
		virtual void flush()=0;
		virtual void flushArea(int x0, int y0, int x1, int y1)=0;
		virtual void highliteArea(int x0, int y0, int x1, int y1){};
};

__END_YAFRAY

#endif // Y_COUTPUT_H
