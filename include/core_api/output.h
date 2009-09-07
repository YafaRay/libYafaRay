/****************************************************************************
 *
 * 			output.h: Generic output module api 
 *      This is part of the yafray package
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
#ifndef Y_COUTPUT_H
#define Y_COUTPUT_H

#include "yafray_constants.h"

// #include "color.h" // to be removed

__BEGIN_YAFRAY

class colorOutput_t
{
	public:
		virtual ~colorOutput_t() {};
//		virtual bool putPixel(int x, int y,const color_t &c, 
//				CFLOAT alpha=0,PFLOAT depth=0)=0;  // to be removed
		virtual bool putPixel(int x, int y, const float *c, int channels)=0;
		virtual void flush()=0;
		virtual void flushArea(int x0, int y0, int x1, int y1)=0;
};

__END_YAFRAY

#endif // Y_COUTPUT_H
