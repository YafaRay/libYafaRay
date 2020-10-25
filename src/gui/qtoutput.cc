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
#include "color/color_layers.h"
#include <QCoreApplication>

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

bool QtOutput::putPixel(int x, int y, const yafaray4::ColorLayer &color_layer)
{
	int r = std::max(0, std::min(255, (int)(color_layer.color_.r_ * 255.f)));
	int g = std::max(0, std::min(255, (int)(color_layer.color_.g_ * 255.f)));
	int b = std::max(0, std::min(255, (int)(color_layer.color_.b_ * 255.f)));
	QRgb aval = Qt::white;
	//QRgb zval = Qt::black;
	QRgb rgb = qRgb(r, g, b);

	int a = std::max(0, std::min(255, (int)(color_layer.color_.a_ * 255.f)));
	aval = qRgb(a, a, a);

	render_buffer_->setPixel(x, y, rgb, aval);

	return true;
}

void QtOutput::flush(const yafaray4::RenderControl &render_control)
{
	QCoreApplication::postEvent(render_buffer_, new GuiUpdateEvent(QRect(), true));
}

void QtOutput::flushArea(int x_0, int y_0, int x_1, int y_1)
{
	QCoreApplication::postEvent(render_buffer_, new GuiUpdateEvent(QRect(x_0, y_0, x_1 - x_0, y_1 - y_0)));
}

void QtOutput::highlightArea(int x_0, int y_0, int x_1, int y_1)
{
	QCoreApplication::postEvent(render_buffer_, new GuiAreaHighliteEvent(QRect(x_0, y_0, x_1 - x_0, y_1 - y_0)));
}
