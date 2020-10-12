/****************************************************************************
 *      renderwidget.h: the widget for displaying the render output
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

#ifndef YAFARAY_RENDERWIDGET_H
#define YAFARAY_RENDERWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QMutex>

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
		void setRenderBorderStart(const QPoint &start) { border_start_ = start; }

		void startRendering();
		bool isRendering() { return rendering_; }
		void finishRendering();

		void setPixel(int x, int y, QRgb color, QRgb alpha);

		void paintColorBuffer();
		void paintAlpha();

	private:
		void zoom(float f, QPoint m_pos);
	public:
		void zoomIn(QPoint m_pos);
		void zoomOut(QPoint m_pos);

		bool event(QEvent *e);
	protected:
		virtual void paintEvent(QPaintEvent *e);
		virtual void wheelEvent(QWheelEvent *evt);
		virtual void mousePressEvent(QMouseEvent *e);
		virtual void mouseReleaseEvent(QMouseEvent *e);
		virtual void mouseMoveEvent(QMouseEvent *e);

	private:
		bool use_zbuf_;
		bool rendering_;
		bool panning_;

		QPoint border_start_;
		QSize image_size_;
		float scale_factor_;

		QPoint pan_pos_;
		QPoint bar_pos_;
		QScrollArea *owner_;
		QScrollBar *h_bar_;
		QScrollBar *v_bar_;

		QPixmap pix_;
		QMutex buffer_mutex_;

		QImage color_buffer_;
		QImage alpha_channel_;
		QImage *active_buffer_;
};

#endif

