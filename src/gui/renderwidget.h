/****************************************************************************
 *      renderwidget.h: the widget for displaying the render output
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

#ifndef Y_RENDERWIDGET_H
#define Y_RENDERWIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QLabel>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>
#include <QtCore/QMutex>

class RenderWidget: public QLabel
{
	Q_OBJECT
public:

	RenderWidget(QScrollArea *parent = 0, bool use_zbuffer = false);
	~RenderWidget();

	void setup(const QSize &s);
private:
	void initBuffers();
public:
	void setRenderBorderStart(const QPoint &start) { borderStart = start; }
	void setRenderDepthMap(bool use_depth) { use_zbuf = use_depth; }

	void startRendering();
	bool isRendering() { return rendering; }
	void finishRendering();

	void setPixel(int x, int y, QRgb color, QRgb alpha, QRgb depth, bool withAlpha, bool withDepth);

	void paintColorBuffer();
	void paintAlpha();
	void paintDepth();

private:
	void zoom(float f, QPoint mPos);
public:
	void zoomIn(QPoint mPos);
	void zoomOut(QPoint mPos);

	bool event(QEvent *e);
protected:
	virtual void paintEvent(QPaintEvent *e);
	virtual void wheelEvent(QWheelEvent* evt);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);

private:
	bool use_zbuf;
	bool rendering;
	bool panning;

	QPoint borderStart;
	QSize imageSize;
	float scaleFactor;

	QPoint panPos;
	QPoint barPos;
	QScrollArea *owner;
	QScrollBar *hBar;
	QScrollBar *vBar;

	QPixmap pix;
	QMutex bufferMutex;

	QImage colorBuffer;
	QImage alphaChannel;
	QImage depthChannel;
	QImage *activeBuffer;
};

#endif

