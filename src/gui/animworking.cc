/****************************************************************************
 *      animworking.cc: a widget to show something is being processed
 *      This is part of the yafray package
 *      Copyright (C) 2009 Gustavo Pichorim Boiko
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


#include "animworking.h"
#include <QtGui/QPainter>
#include <QtGui/QConicalGradient>
#include <QtGui/QImage>

AnimWorking::AnimWorking(QWidget *parent)
	: QWidget(parent), m_rotation(0), m_timerId(-1)
{
	// render it to an image (safer)
	QImage img(64, 64, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::transparent);

	// render the radial gradient
	QConicalGradient g(img.rect().center(), 0);
	g.setColorAt(0., qRgb(184, 0, 0));
	g.setColorAt(1., Qt::transparent);

	QPainter p(&img);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setPen(Qt::NoPen);
	p.setBrush(g);
	p.drawEllipse(img.rect());
	p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
	p.setBrush(Qt::black);
	p.drawEllipse(img.rect().adjusted(10,10,-10,-10));

	m_gradientPix = QPixmap::fromImage(img);
}

AnimWorking::~AnimWorking()
{

}

void AnimWorking::paintEvent(QPaintEvent *e)
{
	if (m_timerId < 0)
		m_timerId = startTimer(50);

	QPointF center(m_gradientPix.width() / 2., m_gradientPix.height() / 2.);
	QPainter p(this);
	p.translate(rect().center() - center);

	p.translate(center);
	p.rotate(m_rotation);
	p.translate(-center);

	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);
	p.drawPixmap(0,0, m_gradientPix);
}

void AnimWorking::timerEvent(QTimerEvent *e)
{
	m_rotation += 15;
	if (m_rotation > 360)
		m_rotation -= 360;
	update();

	if (!isVisible())
	{
		killTimer(m_timerId);
		m_timerId = -1;
	}

}
