/****************************************************************************
 *      qtoutput.cc: a Qt color output for yafray
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

#include "qtoutput.h"
#include "events.h"
#include "common/color.h"
#include <QCoreApplication>
#include <iostream>
#include <cstdlib>

QtOutput::QtOutput(RenderWidget *render): render_buffer_(render)
{
}

void QtOutput::setRenderSize(const QSize &s)
{
	render_buffer_->setup(s);
}

/*====================================
/ colorOutput_t implementations
=====================================*/

bool QtOutput::putPixel(int num_view, int x, int y, const yafaray4::RenderPasses *render_passes, int idx, const yafaray4::Rgba &color, bool alpha)
{
	int r = std::max(0, std::min(255, (int)(color.r_ * 255.f)));
	int g = std::max(0, std::min(255, (int)(color.g_ * 255.f)));
	int b = std::max(0, std::min(255, (int)(color.b_ * 255.f)));
	QRgb aval = Qt::white;
	//QRgb zval = Qt::black;
	QRgb rgb = qRgb(r, g, b);

	if(alpha)
	{
		int a = std::max(0, std::min(255, (int)(color.a_ * 255.f)));
		aval = qRgb(a, a, a);
	}

	render_buffer_->setPixel(x, y, rgb, aval, alpha);

	return true;
}

bool QtOutput::putPixel(int num_view, int x, int y, const yafaray4::RenderPasses *render_passes, const std::vector<yafaray4::Rgba> &col_ext_passes, bool alpha)
{
	int r = std::max(0, std::min(255, (int)(col_ext_passes.at(0).r_ * 255.f)));
	int g = std::max(0, std::min(255, (int)(col_ext_passes.at(0).g_ * 255.f)));
	int b = std::max(0, std::min(255, (int)(col_ext_passes.at(0).b_ * 255.f)));
	QRgb aval = Qt::white;
	//QRgb zval = Qt::black;
	QRgb rgb = qRgb(r, g, b);

	if(alpha)
	{
		int a = std::max(0, std::min(255, (int)(col_ext_passes.at(0).a_ * 255.f)));
		aval = qRgb(a, a, a);
	}

	render_buffer_->setPixel(x, y, rgb, aval, alpha);

	return true;
}

void QtOutput::flush(int num_view, const yafaray4::RenderPasses *render_passes)
{
	QCoreApplication::postEvent(render_buffer_, new GuiUpdateEvent(QRect(), true));
}

void QtOutput::flushArea(int num_view, int x_0, int y_0, int x_1, int y_1, const yafaray4::RenderPasses *render_passes)
{
	QCoreApplication::postEvent(render_buffer_, new GuiUpdateEvent(QRect(x_0, y_0, x_1 - x_0, y_1 - y_0)));
}

void QtOutput::highlightArea(int num_view, int x_0, int y_0, int x_1, int y_1)
{
	QCoreApplication::postEvent(render_buffer_, new GuiAreaHighliteEvent(QRect(x_0, y_0, x_1 - x_0, y_1 - y_0)));
}
