/****************************************************************************
 *      qtoutput.h: a Qt color output for yafray
 *      This is part of the yafray package
 *      Copyright (C) 2008 Gustavo Pichorim Boiko
 *		Copyright (C) 2009 Rodrigo Placencia Vazquez
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
 */

#ifndef Y_QTOUTPUT_H
#define Y_QTOUTPUT_H

#include <core_api/output.h>
#include "renderwidget.h"
#include <QImage>

class QWidget;

class QtOutput: public yafaray::colorOutput_t
{
	public:
		QtOutput(RenderWidget *render);
		~QtOutput() {}

		void setRenderSize(const QSize &s);

		// inherited from yafaray::colorOutput_t
		virtual bool putPixel(int numView, int x, int y, const yafaray::renderPasses_t *renderPasses, int idx, const yafaray::colorA_t &color, bool alpha = true);
		virtual bool putPixel(int numView, int x, int y, const yafaray::renderPasses_t *renderPasses, const std::vector<yafaray::colorA_t> &colExtPasses, bool alpha = true);
		virtual void flush(int numView, const yafaray::renderPasses_t *renderPasses);
		virtual void flushArea(int numView, int x0, int y0, int x1, int y1, const yafaray::renderPasses_t *renderPasses);
		virtual void highliteArea(int numView, int x0, int y0, int x1, int y1);

	private:
		RenderWidget *renderBuffer;
};

#endif
