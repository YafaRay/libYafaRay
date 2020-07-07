#pragma once
/****************************************************************************
 *
 * 		output.h: Output base class
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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
#include <vector>
#include <string>

__BEGIN_YAFRAY

class renderPasses_t;
class colorA_t;

/*! Base class for rendering output containers */

class colorOutput_t
{
	public:
		virtual ~colorOutput_t() {};
        virtual void initTilesPasses(int totalViews, int numExtPasses) {};
        virtual bool putPixel(int numView, int x, int y, const renderPasses_t *renderPasses, int idx, const colorA_t &color, bool alpha = true)=0;
		virtual bool putPixel(int numView, int x, int y, const renderPasses_t *renderPasses, const std::vector<colorA_t> &colExtPasses, bool alpha = true)=0;
		virtual void flush(int numView, const renderPasses_t *renderPasses)=0;
		virtual void flushArea(int numView, int x0, int y0, int x1, int y1, const renderPasses_t *renderPasses)=0;
		virtual void highliteArea(int numView, int x0, int y0, int x1, int y1){};
		virtual bool isImageOutput() { return false; }
		virtual bool isPreview() { return false; }
		virtual std::string getDenoiseParams() const { return ""; }
};

__END_YAFRAY

#endif // Y_COUTPUT_H
