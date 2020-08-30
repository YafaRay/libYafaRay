/****************************************************************************
 *      animworking.h: a widget to show something is being processed
 *      This is part of the libYafaRay package
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


#ifndef YAFARAY_ANIMWORKING_H
#define YAFARAY_ANIMWORKING_H

#include <QWidget>
#include <vector>

#define TOP_FRAME 50

class AnimWorking : public QWidget
{
		Q_OBJECT

	public:
		AnimWorking(QWidget *parent = 0);
		~AnimWorking();
	protected:
		void paintEvent(QPaintEvent *e);
		void timerEvent(QTimerEvent *e);
	private:
		std::vector<QPixmap> m_sprites_;
		size_t m_act_frame_;
		int m_timer_id_;
};

#endif // YAFARAY_ANIMWORKING_H
