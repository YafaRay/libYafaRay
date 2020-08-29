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

#include <QDebug>
#include <QApplication>
#include <QPushButton>
#include <QPainter>
#include <QPaintEvent>
#include "renderwidget.h"
#include "events.h"
#include <iostream>

/*=====================================
/	RenderWidget implementation
/=====================================*/

RenderWidget::RenderWidget(QScrollArea *parent, bool use_zbuffer): QLabel((QWidget *)parent), use_zbuf_(use_zbuffer)
{
	border_start_ = QPoint(0, 0);
	rendering_ = true;
	scale_factor_ = 1.0;
	panning_ = false;
	pan_pos_ = QPoint(0, 0);
	owner_ = parent;
	h_bar_ = owner_->horizontalScrollBar();
	v_bar_ = owner_->verticalScrollBar();
	bar_pos_ = QPoint(0, 0);
	setScaledContents(true);
}

RenderWidget::~RenderWidget()
{
	color_buffer_ = QImage();
	alpha_channel_ = QImage();
}

void RenderWidget::setup(const QSize &s)
{
	image_size_ = s;

	initBuffers();

	QPalette palette;
	palette.setColor(QPalette::Background, QColor(0, 0, 0));
	setPalette(palette);
}

void RenderWidget::initBuffers()
{
	color_buffer_ = QImage(image_size_, QImage::Format_RGB32);
	color_buffer_.fill(0);

	alpha_channel_ = QImage(image_size_, QImage::Format_RGB32);
	alpha_channel_.fill(0);

	resize(image_size_);

	active_buffer_ = &color_buffer_;

	pix_ = QPixmap::fromImage(*active_buffer_);
	setPixmap(pix_);
}

void RenderWidget::startRendering()
{
	rendering_ = true;
	scale_factor_ = 1.0;
	initBuffers();
}

void RenderWidget::finishRendering()
{
	rendering_ = false;
	pix_ = QPixmap::fromImage(*active_buffer_);
	setPixmap(pix_);
	update();
}

void RenderWidget::setPixel(int x, int y, QRgb color, QRgb alpha, bool with_alpha)
{
	int ix = x + border_start_.x();
	int iy = y + border_start_.y();

	color_buffer_.setPixel(ix, iy, color);
	if(with_alpha) alpha_channel_.setPixel(ix, iy, alpha);
}

void RenderWidget::paintColorBuffer()
{
	buffer_mutex_.lock();
	pix_ = QPixmap::fromImage(color_buffer_);
	setPixmap(pix_);
	active_buffer_ = &color_buffer_;
	buffer_mutex_.unlock();
	if(!rendering_) zoom(1.f, QPoint(0, 0));
}

void RenderWidget::paintAlpha()
{
	buffer_mutex_.lock();
	pix_ = QPixmap::fromImage(alpha_channel_);
	setPixmap(pix_);
	active_buffer_ = &alpha_channel_;
	buffer_mutex_.unlock();
	if(!rendering_) zoom(1.f, QPoint(0, 0));
}

void RenderWidget::zoom(float f, QPoint m_pos)
{
	scale_factor_ *= f;

	QSize new_size = scale_factor_ * active_buffer_->size();
	resize(new_size);
	pix_ = QPixmap::fromImage(active_buffer_->scaled(new_size));
	update(owner_->viewport()->geometry());

	QPoint m = (m_pos * f) - m_pos;

	int dh = h_bar_->value() + (m.x());
	int dv = v_bar_->value() + (m.y());

	h_bar_->setValue(dh);
	v_bar_->setValue(dv);
}

void RenderWidget::zoomIn(QPoint m_pos)
{
	if(scale_factor_ > 5.0) return;

	zoom(1.25, m_pos);
}

void RenderWidget::zoomOut(QPoint m_pos)
{
	if(scale_factor_ < 0.2) return;

	zoom(0.8, m_pos);
}

bool RenderWidget::event(QEvent *e)
{
	if(e->type() == (QEvent::Type)GuiUpdate && rendering_)
	{
		GuiUpdateEvent *ge = (GuiUpdateEvent *)e;

		ge->accept();

		if(ge->fullUpdate())
		{
			buffer_mutex_.lock();
			QPainter p(&pix_);
			p.drawImage(QPoint(0, 0), *active_buffer_);
			buffer_mutex_.unlock();
			update();
		}
		else
		{
			buffer_mutex_.lock();
			QPainter p(&pix_);
			p.drawImage(ge->rect(), *active_buffer_, ge->rect());
			buffer_mutex_.unlock();
			update(ge->rect());

		}

		return true;
	}
	else if(e->type() == (QEvent::Type)GuiAreaHighlite && rendering_)
	{
		GuiAreaHighliteEvent *ge = (GuiAreaHighliteEvent *)e;
		buffer_mutex_.lock();
		QPainter p(&pix_);

		ge->accept();

		int line_l = std::min(4, std::min(ge->rect().height() - 1, ge->rect().width() - 1));
		QPoint tr(ge->rect().topRight());
		QPoint tl(ge->rect().topLeft());
		QPoint br(ge->rect().bottomRight());
		QPoint bl(ge->rect().bottomLeft());

		p.setPen(QColor(160, 0, 0));

		//top-left corner
		p.drawLine(tl, QPoint(tl.x() + line_l, tl.y()));
		p.drawLine(tl, QPoint(tl.x(), tl.y() + line_l));

		//top-right corner
		p.drawLine(tr, QPoint(tr.x() - line_l, tr.y()));
		p.drawLine(tr, QPoint(tr.x(), tr.y() + line_l));

		//bottom-left corner
		p.drawLine(bl, QPoint(bl.x() + line_l, bl.y()));
		p.drawLine(bl, QPoint(bl.x(), bl.y() - line_l));

		//bottom-right corner
		p.drawLine(br, QPoint(br.x() - line_l, br.y()));
		p.drawLine(br, QPoint(br.x(), br.y() - line_l));

		buffer_mutex_.unlock();
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
	painter.drawPixmap(r, pix_, r);
}

void RenderWidget::wheelEvent(QWheelEvent *e)
{
	e->accept();

	if(!rendering_ && !panning_ && (e->modifiers() & Qt::ControlModifier))
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
		panning_ = true;
		pan_pos_ = e->globalPos();
		bar_pos_ = QPoint(h_bar_->value(), v_bar_->value());
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
		panning_ = false;
		e->accept();
	}
	else
	{
		e->ignore();
	}
}

void RenderWidget::mouseMoveEvent(QMouseEvent *e)
{
	if(panning_)
	{
		QPoint dpos = bar_pos_ + (pan_pos_ - e->globalPos());
		h_bar_->setValue(dpos.x());
		v_bar_->setValue(dpos.y());
		e->accept();
	}
	else
	{
		e->ignore();
	}
}
