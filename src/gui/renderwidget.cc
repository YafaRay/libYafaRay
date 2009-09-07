/****************************************************************************
 *      renderwidget.cc: a widget for displaying the rendering output
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

RenderWidget::RenderWidget(QScrollArea *parent): QLabel((QWidget*)parent) {
	borderStart = QPoint(0, 0);
	rendering = true;
	scaleFactor = 1.0;
	panning = false;
	panPos = QPoint(0, 0);
	owner = parent;
	hBar = owner->horizontalScrollBar();
	vBar = owner->verticalScrollBar();
	barPos = QPoint(0, 0);
}

bool RenderWidget::event(QEvent *e)
{
	if (e->type() == (QEvent::Type)GuiUpdate)
	{
		GuiUpdateEvent *ge = (GuiUpdateEvent*)e;
		if (ge->fullUpdate())
		{
			img = ge->img();
			pix = QPixmap::fromImage(ge->img());
			update();
		}
		else
		{
			QPainter p;
			p.begin(&pix);
			p.drawImage(ge->rect(), ge->img(), ge->rect());
			update(ge->rect());
			p.end();

			p.begin(&img);
			p.drawImage(ge->rect(), ge->img(), ge->rect());
			p.end();
		}
		return true;
	}
	return QLabel::event(e);
}

void RenderWidget::paintEvent(QPaintEvent *e)
{
	if (rendering) {
		QRect r = e->rect();
		QPainter painter(this);
		painter.setClipRegion(e->region());

		//if (pixmap.isNull()) {
		if (pix.isNull()) {
			painter.fillRect(r, Qt::black);
			painter.setPen(Qt::white);
			painter.drawText(rect(), Qt::AlignCenter, tr("<no image data>"));
			return;
		}
		painter.drawPixmap(r, pix, r);
	}
	else {
		QLabel::paintEvent(e);
	}
}

bool RenderWidget::saveImage(const QString &path, bool alpha)
{
	QImage image(img);
	if (alpha) {
		/*
		QPainter p(&image);
		p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		p.drawImage(0, 0, alphaChannel);
		TODO: the 8-bit greyscale colors need to be copied to the alpha channel somehow
		      for now, use the obsolete setAlphaChannel() method
		*/
		image.setAlphaChannel(alphaChannel);
	}
	return image.save(path, 0);
}

void RenderWidget::finishedRender() {
	rendering = false;
	setPixmap(pix);
}

void RenderWidget::zoom(float f, QPoint mPos)
{
	scaleFactor *= f;
	
	Qt::TransformationMode trasMode = Qt::FastTransformation;
	if(scaleFactor <= 1.25) trasMode = Qt::SmoothTransformation;

	QSize newSize = scaleFactor * pix.size();
	
	setPixmap(pix.scaled(newSize, Qt::KeepAspectRatio, trasMode));
	resize(newSize);
	update();

	QPoint m = (mPos * f) - mPos;
	
	int dh = hBar->value() + (m.x());
	int dv = vBar->value() + (m.y());

	hBar->setValue(dh);
	vBar->setValue(dv);
}

void RenderWidget::zoomIn(QPoint mPos)
{
	if(scaleFactor > 5.0 || rendering) return;
	
	zoom(1.25, mPos);
}

void RenderWidget::zoomOut(QPoint mPos)
{
	if(scaleFactor < 0.2 || rendering) return;
	
	zoom(0.8, mPos);
}

void RenderWidget::wheelEvent(QWheelEvent* e)
{
	if(!rendering && !panning && (e->modifiers() & Qt::ControlModifier))
	{
		if(e->delta() > 0) zoomIn(e->pos());
		else zoomOut(e->pos());
	}

	e->accept();
}

void RenderWidget::mousePressEvent(QMouseEvent *e)
{
	if(e->button() == Qt::MidButton)
	{
		e->accept();
		setCursor(Qt::SizeAllCursor);
		panning = true;
		panPos = e->globalPos();
		barPos = QPoint(hBar->value(), vBar->value());
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
		e->accept();
		setCursor(Qt::ArrowCursor);
		panning = false;
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
		e->accept();
		QPoint dpos = barPos + (panPos - e->globalPos());
		hBar->setValue(dpos.x());
		vBar->setValue(dpos.y());
	}
	else
	{
		e->ignore();
	}
}
