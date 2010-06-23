/****************************************************************************
 *      animworking.h: a widget to show something is being processed
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


#ifndef ANIMWORKING_H
#define ANIMWORKING_H

#include <QtGui/QWidget>
#include <vector>

#define topFrame 50

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
	std::vector<QPixmap> mSprites;
	size_t mActFrame;
	int m_timerId;
};

#endif // ANIMWORKING_H
