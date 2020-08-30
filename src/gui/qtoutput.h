/****************************************************************************
 *      qtoutput.h: a Qt color output for yafray
 *      This is part of the libYafaRay package
 *      Copyright (C) 2008 Gustavo Pichorim Boiko
 *      Copyright (C) 2009 Rodrigo Placencia Vazquez
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

#ifndef YAFARAY_QTOUTPUT_H
#define YAFARAY_QTOUTPUT_H

#include "output/output.h"
#include "renderwidget.h"
#include <QImage>

class QWidget;

class QtOutput: public yafaray4::ColorOutput
{
	public:
		QtOutput(RenderWidget *render);
		~QtOutput() {}

		void setRenderSize(const QSize &s);

		// inherited from yafaray4::colorOutput_t
		virtual bool putPixel(int num_view, int x, int y, const yafaray4::RenderPasses *render_passes, int idx, const yafaray4::Rgba &color, bool alpha = true);
		virtual bool putPixel(int num_view, int x, int y, const yafaray4::RenderPasses *render_passes, const std::vector<yafaray4::Rgba> &col_ext_passes, bool alpha = true);
		virtual void flush(int num_view, const yafaray4::RenderPasses *render_passes);
		virtual void flushArea(int num_view, int x_0, int y_0, int x_1, int y_1, const yafaray4::RenderPasses *render_passes);
		virtual void highlightArea(int num_view, int x_0, int y_0, int x_1, int y_1);

	private:
		RenderWidget *render_buffer_;
};

#endif
