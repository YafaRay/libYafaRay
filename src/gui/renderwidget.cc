/****************************************************************************
 *      renderwidget.cc: a widget for displaying the rendering output
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

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtGui/QPushButton>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include "renderwidget.h"
#include "events.h"
#include <iostream>

/*=====================================
/	RenderWidget implementation
/=====================================*/

RenderWidget::RenderWidget(QScrollArea *parent, bool use_zbuffer): QLabel((QWidget*)parent), use_zbuf(use_zbuffer) {
	borderStart = QPoint(0, 0);
	rendering = true;
	scaleFactor = 1.0;
	panning = false;
	panPos = QPoint(0, 0);
	owner = parent;
	hBar = owner->horizontalScrollBar();
	vBar = owner->verticalScrollBar();
	barPos = QPoint(0, 0);
	setScaledContents(true);
}

RenderWidget::~RenderWidget()
{
	colorBuffer = QImage();
	alphaChannel = QImage();
	depthChannel = QImage();
}

void RenderWidget::setup(const QSize &s)
{
	imageSize = s;

	initBuffers();

	QPalette palette;
	palette.setColor(QPalette::Background, QColor(0, 0, 0));
	setPalette(palette);
}

void RenderWidget::initBuffers()
{
	colorBuffer = QImage(imageSize, QImage::Format_RGB32);
	colorBuffer.fill(0);

	alphaChannel = QImage(imageSize, QImage::Format_RGB32);
	alphaChannel.fill(0);

	if(use_zbuf)
	{
		depthChannel = QImage(imageSize, QImage::Format_RGB32);
		depthChannel.fill(0);
	}

	resize(imageSize);

	activeBuffer = &colorBuffer;

	pix = QPixmap::fromImage(*activeBuffer);
	setPixmap(pix);
}

void RenderWidget::startRendering()
{
	rendering = true;
	scaleFactor = 1.0;
	initBuffers();
}

void RenderWidget::finishRendering()
{
	rendering = false;
	pix = QPixmap::fromImage(*activeBuffer);
	setPixmap(pix);
	update();
}

void RenderWidget::setPixel(int x, int y, QRgb color, QRgb alpha, QRgb depth, bool withAlpha, bool withDepth)
{
	int ix = x + borderStart.x();
	int iy = y + borderStart.y();

	colorBuffer.setPixel(ix, iy, color);
	if (withAlpha) alphaChannel.setPixel(ix, iy, alpha);
	if (withDepth) depthChannel.setPixel(ix, iy, depth);
}

void RenderWidget::paintColorBuffer()
{
	bufferMutex.lock();
	pix = QPixmap::fromImage(colorBuffer);
	setPixmap(pix);
	activeBuffer = &colorBuffer;
	bufferMutex.unlock();
	if(!rendering) zoom(1.f, QPoint(0, 0));
}

void RenderWidget::paintAlpha()
{
	bufferMutex.lock();
	pix = QPixmap::fromImage(alphaChannel);
	setPixmap(pix);
	activeBuffer = &alphaChannel;
	bufferMutex.unlock();
	if(!rendering) zoom(1.f, QPoint(0, 0));
}

void RenderWidget::paintDepth()
{
	if(use_zbuf)
	{
		bufferMutex.lock();
		pix = QPixmap::fromImage(depthChannel);
		setPixmap(pix);
		activeBuffer = &depthChannel;
		bufferMutex.unlock();
		if(!rendering) zoom(1.f, QPoint(0, 0));
	}
}

void RenderWidget::zoom(float f, QPoint mPos)
{
	scaleFactor *= f;

	QSize newSize = scaleFactor * activeBuffer->size();
	resize(newSize);
	pix = QPixmap::fromImage(activeBuffer->scaled(newSize));
	update(owner->viewport()->geometry());

	QPoint m = (mPos * f) - mPos;

	int dh = hBar->value() + (m.x());
	int dv = vBar->value() + (m.y());

	hBar->setValue(dh);
	vBar->setValue(dv);
}

void RenderWidget::zoomIn(QPoint mPos)
{
	if(scaleFactor > 5.0) return;

	zoom(1.25, mPos);
}

void RenderWidget::zoomOut(QPoint mPos)
{
	if(scaleFactor < 0.2) return;

	zoom(0.8, mPos);
}

bool RenderWidget::event(QEvent *e)
{
	if (e->type() == (QEvent::Type)GuiUpdate && rendering)
	{
		GuiUpdateEvent *ge = (GuiUpdateEvent*)e;

		ge->accept();

		if (ge->fullUpdate())
		{
			bufferMutex.lock();
			QPainter p(&pix);
			p.drawImage(QPoint(0, 0), *activeBuffer);
			bufferMutex.unlock();
			update();
		}
		else
		{
			bufferMutex.lock();
			QPainter p(&pix);
			p.drawImage(ge->rect(), *activeBuffer, ge->rect());
			bufferMutex.unlock();
			update(ge->rect());

		}

		return true;
	}
	else if (e->type() == (QEvent::Type)GuiAreaHighlite && rendering)
	{
		GuiAreaHighliteEvent *ge = (GuiAreaHighliteEvent*)e;
		bufferMutex.lock();
		QPainter p(&pix);

		ge->accept();

		int lineL = std::min( 4, std::min( ge->rect().height()-1, ge->rect().width()-1 ) );
		QPoint tr(ge->rect().topRight());
		QPoint tl(ge->rect().topLeft());
		QPoint br(ge->rect().bottomRight());
		QPoint bl(ge->rect().bottomLeft());

		p.setPen(QColor(160, 0, 0));

		//top-left corner
		p.drawLine(tl, QPoint(tl.x() + lineL, tl.y()));
		p.drawLine(tl, QPoint(tl.x(), tl.y() + lineL));

		//top-right corner
		p.drawLine(tr, QPoint(tr.x() - lineL, tr.y()));
		p.drawLine(tr, QPoint(tr.x(), tr.y() + lineL));

		//bottom-left corner
		p.drawLine(bl, QPoint(bl.x() + lineL, bl.y()));
		p.drawLine(bl, QPoint(bl.x(), bl.y() - lineL));

		//bottom-right corner
		p.drawLine(br, QPoint(br.x() - lineL, br.y()));
		p.drawLine(br, QPoint(br.x(), br.y() - lineL));

		bufferMutex.unlock();
		update(ge->rect());

		return true;
	}

	return QLabel::event(e);
}

void RenderWidget::paintEvent(QPaintEvent *e)
{
    QRect r = e->rect();
    QPainter painter(this);
    painter.setClipRegion(e->region());
    painter.drawPixmap(r, pix, r);
}

void RenderWidget::wheelEvent(QWheelEvent* e)
{
	e->accept();

	if(!rendering && !panning && (e->modifiers() & Qt::ControlModifier))
	{
		if(e->delta() > 0) zoomIn(e->pos());
		else zoomOut(e->pos());
	}
}

void RenderWidget::mousePressEvent(QMouseEvent *e)
{
	if(e->button() == Qt::MidButton)
	{
		setCursor(Qt::SizeAllCursor);
		panning = true;
		panPos = e->globalPos();
		barPos = QPoint(hBar->value(), vBar->value());
		e->accept();
	}
	else
	{
		e->ignore();
	}
}

void RenderWidget::mouseReleaseEvent(QMouseEvent *e)
{
	if(e->button() == Qt::MidButton)
	{
		setCursor(Qt::ArrowCursor);
		panning = false;
		e->accept();
	}
	else
	{
		e->ignore();
	}
}

void RenderWidget::mouseMoveEvent(QMouseEvent *e)
{
	if(panning)
	{
		QPoint dpos = barPos + (panPos - e->globalPos());
		hBar->setValue(dpos.x());
		vBar->setValue(dpos.y());
		e->accept();
	}
	else
	{
		e->ignore();
	}
}
