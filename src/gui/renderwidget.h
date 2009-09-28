/****************************************************************************
 *      renderwidget.h: the widget for displaying the render output
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

#ifndef Y_RENDERWIDGET_H
#define Y_RENDERWIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QLabel>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>

class RenderWidget: public QLabel
{
	Q_OBJECT
public:
	RenderWidget(QScrollArea *parent = 0);
	~RenderWidget(){};

	bool saveImage(const QString &path, bool alpha);
	void finishedRender();

	bool event(QEvent *e);
	
	void zoomIn(QPoint mPos);
	void zoomOut(QPoint mPos);
	
	bool isRendering() { return rendering; }
	void startRendering() { rendering = true; scaleFactor = 1.0; }

	//bool useAlpha();
	//void setUseAlpha(bool use);
	//QImage removeAlpha(const QImage &img);
	QImage img;
	QImage alphaChannel;
	QPixmap pix;
	QPoint borderStart;
protected:
	virtual void paintEvent(QPaintEvent *e);
	virtual void wheelEvent(QWheelEvent* evt);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);

private:
	bool m_alpha;
	bool rendering;
	void zoom(float f, QPoint mPos);
	float scaleFactor;
	bool panning;
	QPoint panPos;
	QScrollArea *owner;
	QScrollBar *hBar;
	QScrollBar *vBar;
	QPoint barPos;
};

#endif

