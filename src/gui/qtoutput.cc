/****************************************************************************
 *      qtoutput.cc: a Qt color output for yafray
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

#include "qtoutput.h"
#include "events.h"
#include <QCoreApplication>
#include <iostream>
#include <cstdlib>

QtOutput::QtOutput(RenderWidget *render): renderBuffer(render)
{
}

void QtOutput::setRenderSize(const QSize &s)
{
	renderBuffer->setup(s);
}

/*====================================
/ colorOutput_t implementations
=====================================*/

bool QtOutput::putPixel(int numView, int x, int y, const yafaray::renderPasses_t *renderPasses, int idx, const yafaray::colorA_t &color, bool alpha)
{
	int r = std::max(0,std::min(255, (int)(color.R * 255.f)));
	int g = std::max(0,std::min(255, (int)(color.G * 255.f)));	
	int b = std::max(0,std::min(255, (int)(color.B * 255.f)));
	QRgb aval = Qt::white;
	//QRgb zval = Qt::black;
	QRgb rgb = qRgb(r, g, b);

	if (alpha)
	{
		int a = std::max(0,std::min(255, (int)(color.A * 255.f)));
		aval = qRgb(a, a, a);
	}
	
	renderBuffer->setPixel(x, y, rgb, aval, alpha);

	return true;
}

bool QtOutput::putPixel(int numView, int x, int y, const yafaray::renderPasses_t *renderPasses, const std::vector<yafaray::colorA_t> &colExtPasses, bool alpha)
{
	int r = std::max(0,std::min(255, (int)(colExtPasses.at(0).R * 255.f)));
	int g = std::max(0,std::min(255, (int)(colExtPasses.at(0).G * 255.f)));	
	int b = std::max(0,std::min(255, (int)(colExtPasses.at(0).B * 255.f)));
	QRgb aval = Qt::white;
	//QRgb zval = Qt::black;
	QRgb rgb = qRgb(r, g, b);

	if (alpha)
	{
		int a = std::max(0,std::min(255, (int)(colExtPasses.at(0).A * 255.f)));
		aval = qRgb(a, a, a);
	}
	
	renderBuffer->setPixel(x, y, rgb, aval, alpha);

	return true;
}

void QtOutput::flush(int numView, const yafaray::renderPasses_t *renderPasses)
{
	QCoreApplication::postEvent(renderBuffer, new GuiUpdateEvent(QRect(), true));
}

void QtOutput::flushArea(int numView, int x0, int y0, int x1, int y1, const yafaray::renderPasses_t *renderPasses)
{
	QCoreApplication::postEvent(renderBuffer, new GuiUpdateEvent(QRect(x0,y0,x1-x0,y1-y0)));
}

void QtOutput::highliteArea(int numView, int x0, int y0, int x1, int y1)
{
	QCoreApplication::postEvent(renderBuffer, new GuiAreaHighliteEvent(QRect(x0,y0,x1-x0,y1-y0)));
}
