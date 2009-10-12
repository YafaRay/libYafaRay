/****************************************************************************
 *      qtoutput.h: a Qt color output for yafray
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

#ifndef Y_QTOUTPUT_H
#define Y_QTOUTPUT_H

#include <core_api/output.h>
#include "renderwidget.h"
#include <QtGui/QImage>

class QWidget;

class QtOutput: public yafaray::colorOutput_t
{
public:
	QtOutput(RenderWidget *render);
	~QtOutput() {}

	void setRenderSize(const QSize &s);
	void clear();

	//bool saveImage(const QString &path);

	// reimplemented from yafaray
	virtual bool putPixel(int x, int y, const float *c, int channels);
	virtual void flush();
	virtual void flushArea(int x0, int y0, int x1, int y1);
	virtual void highliteArea(int x0, int y0, int x1, int y1);

private:
	RenderWidget *widgy;
	bool m_alpha;
	QImage img;
};

#endif
