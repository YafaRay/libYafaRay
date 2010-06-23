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

// Animation Resources

#include <resources/qtanim/00001.h>
#include <resources/qtanim/00002.h>
#include <resources/qtanim/00003.h>
#include <resources/qtanim/00004.h>
#include <resources/qtanim/00005.h>
#include <resources/qtanim/00006.h>
#include <resources/qtanim/00007.h>
#include <resources/qtanim/00008.h>
#include <resources/qtanim/00009.h>
#include <resources/qtanim/00010.h>
#include <resources/qtanim/00011.h>
#include <resources/qtanim/00012.h>
#include <resources/qtanim/00013.h>
#include <resources/qtanim/00014.h>
#include <resources/qtanim/00015.h>
#include <resources/qtanim/00016.h>
#include <resources/qtanim/00017.h>
#include <resources/qtanim/00018.h>
#include <resources/qtanim/00019.h>
#include <resources/qtanim/00020.h>
#include <resources/qtanim/00021.h>
#include <resources/qtanim/00022.h>
#include <resources/qtanim/00023.h>
#include <resources/qtanim/00024.h>
#include <resources/qtanim/00025.h>
#include <resources/qtanim/00026.h>
#include <resources/qtanim/00027.h>
#include <resources/qtanim/00028.h>
#include <resources/qtanim/00029.h>
#include <resources/qtanim/00030.h>
#include <resources/qtanim/00031.h>
#include <resources/qtanim/00032.h>
#include <resources/qtanim/00033.h>
#include <resources/qtanim/00034.h>
#include <resources/qtanim/00035.h>
#include <resources/qtanim/00036.h>
#include <resources/qtanim/00037.h>
#include <resources/qtanim/00038.h>
#include <resources/qtanim/00039.h>
#include <resources/qtanim/00040.h>
#include <resources/qtanim/00041.h>
#include <resources/qtanim/00042.h>
#include <resources/qtanim/00043.h>
#include <resources/qtanim/00044.h>
#include <resources/qtanim/00045.h>
#include <resources/qtanim/00046.h>
#include <resources/qtanim/00047.h>
#include <resources/qtanim/00048.h>
#include <resources/qtanim/00049.h>
#include <resources/qtanim/00050.h>

AnimWorking::AnimWorking(QWidget *parent)
	: QWidget(parent), m_timerId(-1)
{
	mSprites.resize(topFrame);
	mSprites[0].loadFromData(sprite00001, sprite00001_size);
	mSprites[1].loadFromData(sprite00002, sprite00002_size);
	mSprites[2].loadFromData(sprite00003, sprite00003_size);
	mSprites[3].loadFromData(sprite00004, sprite00004_size);
	mSprites[4].loadFromData(sprite00005, sprite00005_size);
	mSprites[5].loadFromData(sprite00006, sprite00006_size);
	mSprites[6].loadFromData(sprite00007, sprite00007_size);
	mSprites[7].loadFromData(sprite00008, sprite00008_size);
	mSprites[8].loadFromData(sprite00009, sprite00009_size);
	mSprites[9].loadFromData(sprite00010, sprite00010_size);

	mSprites[10].loadFromData(sprite00011, sprite00011_size);
	mSprites[11].loadFromData(sprite00012, sprite00012_size);
	mSprites[12].loadFromData(sprite00013, sprite00013_size);
	mSprites[13].loadFromData(sprite00014, sprite00014_size);
	mSprites[14].loadFromData(sprite00015, sprite00015_size);
	mSprites[15].loadFromData(sprite00016, sprite00016_size);
	mSprites[16].loadFromData(sprite00017, sprite00017_size);
	mSprites[17].loadFromData(sprite00018, sprite00018_size);
	mSprites[18].loadFromData(sprite00019, sprite00019_size);
	mSprites[19].loadFromData(sprite00020, sprite00020_size);

	mSprites[20].loadFromData(sprite00021, sprite00021_size);
	mSprites[21].loadFromData(sprite00022, sprite00022_size);
	mSprites[22].loadFromData(sprite00023, sprite00023_size);
	mSprites[23].loadFromData(sprite00024, sprite00024_size);
	mSprites[24].loadFromData(sprite00025, sprite00025_size);
	mSprites[25].loadFromData(sprite00026, sprite00026_size);
	mSprites[26].loadFromData(sprite00027, sprite00027_size);
	mSprites[27].loadFromData(sprite00028, sprite00028_size);
	mSprites[28].loadFromData(sprite00029, sprite00029_size);
	mSprites[29].loadFromData(sprite00030, sprite00030_size);

	mSprites[30].loadFromData(sprite00031, sprite00031_size);
	mSprites[31].loadFromData(sprite00032, sprite00032_size);
	mSprites[32].loadFromData(sprite00033, sprite00033_size);
	mSprites[33].loadFromData(sprite00034, sprite00034_size);
	mSprites[34].loadFromData(sprite00035, sprite00035_size);
	mSprites[35].loadFromData(sprite00036, sprite00036_size);
	mSprites[36].loadFromData(sprite00037, sprite00037_size);
	mSprites[37].loadFromData(sprite00038, sprite00038_size);
	mSprites[38].loadFromData(sprite00039, sprite00039_size);
	mSprites[39].loadFromData(sprite00040, sprite00040_size);

	mSprites[40].loadFromData(sprite00041, sprite00041_size);
	mSprites[41].loadFromData(sprite00042, sprite00042_size);
	mSprites[42].loadFromData(sprite00043, sprite00043_size);
	mSprites[43].loadFromData(sprite00044, sprite00044_size);
	mSprites[44].loadFromData(sprite00045, sprite00045_size);
	mSprites[45].loadFromData(sprite00046, sprite00046_size);
	mSprites[46].loadFromData(sprite00047, sprite00047_size);
	mSprites[47].loadFromData(sprite00048, sprite00048_size);
	mSprites[48].loadFromData(sprite00049, sprite00049_size);
	mSprites[49].loadFromData(sprite00050, sprite00050_size);
	mActFrame = 0;
}

AnimWorking::~AnimWorking()
{
	mSprites.clear();
}

void AnimWorking::paintEvent(QPaintEvent *e)
{
	if (m_timerId < 0)
		m_timerId = startTimer(40);

	QPainter p(this);
	p.drawPixmap(0,0, mSprites[mActFrame]);
}

void AnimWorking::timerEvent(QTimerEvent *e)
{
	if(mActFrame < topFrame - 1) mActFrame++;
	else mActFrame = 0;
	update();

	if (!isVisible())
	{
		killTimer(m_timerId);
		m_timerId = -1;
	}

}
