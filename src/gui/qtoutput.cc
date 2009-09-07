/****************************************************************************
 *      qtoutput.cc: a Qt color output for yafray
 *      This is part of the yafray package
 *      Copyright (C) 2008 Gustavo Pichorim Boiko
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
#include <QtCore/QCoreApplication>
#include <iostream>
#include <cstdlib>

QtOutput::QtOutput(RenderWidget *render): widgy(render)
{ }

void QtOutput::setRenderSize(const QSize &s)
{
	img = QImage(s, QImage::Format_ARGB32);
	img.fill(0);
	// calling the alphaChannel() function will return a 8-bit indexed 
	// greyscale image
	widgy->alphaChannel = img.alphaChannel();
	widgy->resize(s);
	QPalette palette;
	palette.setColor(QPalette::Background, QColor(0, 0, 0));
	widgy->setPalette(palette);
	// force pixmap creation
	flush();
}

void QtOutput::clear()
{
	// clear image...
	img = QImage();
	widgy->alphaChannel = QImage();
	flush();
}

/*====================================
/ colorOutput_t implementations
=====================================*/

bool QtOutput::putPixel(int x, int y, const float *c, int channels)
{
	int r, g, b, a = 255;

	r = (c[0]<0.f) ? 0 : ((c[0]>=1.f) ? 255 : (unsigned char)(255.f*c[0]) );
	g = (c[1]<0.f) ? 0 : ((c[1]>=1.f) ? 255 : (unsigned char)(255.f*c[1]) );
	b = (c[2]<0.f) ? 0 : ((c[2]>=1.f) ? 255 : (unsigned char)(255.f*c[2]) );
	QRgb rgb = qRgb(r, g, b);
	if (channels > 3)
	{
		a = (int)(c[3]*255);
		if (a > 255) a = 255;
		if (a < 0) a = 0;
	}

	img.setPixel(x + widgy->borderStart.x(),y + widgy->borderStart.y(), rgb);
	//widgy->alphaChannel.setPixel(x + widgy->borderStart.x(), y + widgy->borderStart.y(), a);
	//widgy->alphaChannel.scanLine(y + widgy->borderStart.y())[x + widgy->borderStart.x()] = a;
	widgy->alphaChannel.bits()[widgy->alphaChannel.bytesPerLine() * (y + widgy->borderStart.y()) + x + widgy->borderStart.x()] = a;

	return true;
}

void QtOutput::flush()
{
	QImage tmp = img.copy();
	QCoreApplication::postEvent(widgy, new GuiUpdateEvent(QRect(), tmp, true));
}

void QtOutput::flushArea(int x0, int y0, int x1, int y1)
{
	// adjust borders
	x0 += widgy->borderStart.x();
	x1 += widgy->borderStart.x();
	y0 += widgy->borderStart.y();
	y1 += widgy->borderStart.y();
	// as the tile is finished, it looks like it is safe to use the image instead of
	// copying the tiles for updating
	QCoreApplication::postEvent(widgy, new GuiUpdateEvent(QRect(x0,y0,x1-x0,y1-y0), img));
}
