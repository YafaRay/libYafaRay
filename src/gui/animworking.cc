/****************************************************************************
 *      animworking.cc: a widget to show something is being processed
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


#include "animworking.h"
#include <QPainter>
#include <QConicalGradient>
#include <QImage>

// Animation Resources

#include "resource/qtanim/00001.h"
#include "resource/qtanim/00002.h"
#include "resource/qtanim/00003.h"
#include "resource/qtanim/00004.h"
#include "resource/qtanim/00005.h"
#include "resource/qtanim/00006.h"
#include "resource/qtanim/00007.h"
#include "resource/qtanim/00008.h"
#include "resource/qtanim/00009.h"
#include "resource/qtanim/00010.h"
#include "resource/qtanim/00011.h"
#include "resource/qtanim/00012.h"
#include "resource/qtanim/00013.h"
#include "resource/qtanim/00014.h"
#include "resource/qtanim/00015.h"
#include "resource/qtanim/00016.h"
#include "resource/qtanim/00017.h"
#include "resource/qtanim/00018.h"
#include "resource/qtanim/00019.h"
#include "resource/qtanim/00020.h"
#include "resource/qtanim/00021.h"
#include "resource/qtanim/00022.h"
#include "resource/qtanim/00023.h"
#include "resource/qtanim/00024.h"
#include "resource/qtanim/00025.h"
#include "resource/qtanim/00026.h"
#include "resource/qtanim/00027.h"
#include "resource/qtanim/00028.h"
#include "resource/qtanim/00029.h"
#include "resource/qtanim/00030.h"
#include "resource/qtanim/00031.h"
#include "resource/qtanim/00032.h"
#include "resource/qtanim/00033.h"
#include "resource/qtanim/00034.h"
#include "resource/qtanim/00035.h"
#include "resource/qtanim/00036.h"
#include "resource/qtanim/00037.h"
#include "resource/qtanim/00038.h"
#include "resource/qtanim/00039.h"
#include "resource/qtanim/00040.h"
#include "resource/qtanim/00041.h"
#include "resource/qtanim/00042.h"
#include "resource/qtanim/00043.h"
#include "resource/qtanim/00044.h"
#include "resource/qtanim/00045.h"
#include "resource/qtanim/00046.h"
#include "resource/qtanim/00047.h"
#include "resource/qtanim/00048.h"
#include "resource/qtanim/00049.h"
#include "resource/qtanim/00050.h"

AnimWorking::AnimWorking(QWidget *parent)
	: QWidget(parent), m_timer_id_(-1)
{
	m_sprites_.resize(TOP_FRAME);
	m_sprites_[0].loadFromData(sprite_00001_global, sprite_00001_size_global);
	m_sprites_[1].loadFromData(sprite_00002_global, sprite_00002_size_global);
	m_sprites_[2].loadFromData(sprite_00003_global, sprite_00003_size_global);
	m_sprites_[3].loadFromData(sprite_00004_global, sprite_00004_size_global);
	m_sprites_[4].loadFromData(sprite_00005_global, sprite_00005_size_global);
	m_sprites_[5].loadFromData(sprite_00006_global, sprite_00006_size_global);
	m_sprites_[6].loadFromData(sprite_00007_global, sprite_00007_size_global);
	m_sprites_[7].loadFromData(sprite_00008_global, sprite_00008_size_global);
	m_sprites_[8].loadFromData(sprite_00009_global, sprite_00009_size_global);
	m_sprites_[9].loadFromData(sprite_00010_global, sprite_00010_size_global);

	m_sprites_[10].loadFromData(sprite_00011_global, sprite_00011_size_global);
	m_sprites_[11].loadFromData(sprite_00012_global, sprite_00012_size_global);
	m_sprites_[12].loadFromData(sprite_00013_global, sprite_00013_size_global);
	m_sprites_[13].loadFromData(sprite_00014_global, sprite_00014_size_global);
	m_sprites_[14].loadFromData(sprite_00015_global, sprite_00015_size_global);
	m_sprites_[15].loadFromData(sprite_00016_global, sprite_00016_size_global);
	m_sprites_[16].loadFromData(sprite_00017_global, sprite_00017_size_global);
	m_sprites_[17].loadFromData(sprite_00018_global, sprite_00018_size_global);
	m_sprites_[18].loadFromData(sprite_00019_global, sprite_00019_size_global);
	m_sprites_[19].loadFromData(sprite_00020_global, sprite_00020_size_global);

	m_sprites_[20].loadFromData(sprite_00021_global, sprite_00021_size_global);
	m_sprites_[21].loadFromData(sprite_00022_global, sprite_00022_size_global);
	m_sprites_[22].loadFromData(sprite_00023_global, sprite_00023_size_global);
	m_sprites_[23].loadFromData(sprite_00024_global, sprite_00024_size_global);
	m_sprites_[24].loadFromData(sprite_00025_global, sprite_00025_size_global);
	m_sprites_[25].loadFromData(sprite_00026_global, sprite_00026_size_global);
	m_sprites_[26].loadFromData(sprite_00027_global, sprite_00027_size_global);
	m_sprites_[27].loadFromData(sprite_00028_global, sprite_00028_size_global);
	m_sprites_[28].loadFromData(sprite_00029_global, sprite_00029_size_global);
	m_sprites_[29].loadFromData(sprite_00030_global, sprite_00030_size_global);

	m_sprites_[30].loadFromData(sprite_00031_global, sprite_00031_size_global);
	m_sprites_[31].loadFromData(sprite_00032_global, sprite_00032_size_global);
	m_sprites_[32].loadFromData(sprite_00033_global, sprite_00033_size_global);
	m_sprites_[33].loadFromData(sprite_00034_global, sprite_00034_size_global);
	m_sprites_[34].loadFromData(sprite_00035_global, sprite_00035_size_global);
	m_sprites_[35].loadFromData(sprite_00036_global, sprite_00036_size_global);
	m_sprites_[36].loadFromData(sprite_00037_global, sprite_00037_size_global);
	m_sprites_[37].loadFromData(sprite_00038_global, sprite_00038_size_global);
	m_sprites_[38].loadFromData(sprite_00039_global, sprite_00039_size_global);
	m_sprites_[39].loadFromData(sprite_00040_global, sprite_00040_size_global);

	m_sprites_[40].loadFromData(sprite_00041_global, sprite_00041_size_global);
	m_sprites_[41].loadFromData(sprite_00042_global, sprite_00042_size_global);
	m_sprites_[42].loadFromData(sprite_00043_global, sprite_00043_size_global);
	m_sprites_[43].loadFromData(sprite_00044_global, sprite_00044_size_global);
	m_sprites_[44].loadFromData(sprite_00045_global, sprite_00045_size_global);
	m_sprites_[45].loadFromData(sprite_00046_global, sprite_00046_size_global);
	m_sprites_[46].loadFromData(sprite_00047_global, sprite_00047_size_global);
	m_sprites_[47].loadFromData(sprite_00048_global, sprite_00048_size_global);
	m_sprites_[48].loadFromData(sprite_00049_global, sprite_00049_size_global);
	m_sprites_[49].loadFromData(sprite_00050_global, sprite_00050_size_global);
	m_act_frame_ = 0;
}

AnimWorking::~AnimWorking()
{
	m_sprites_.clear();
}

void AnimWorking::paintEvent(QPaintEvent *e)
{
	if(m_timer_id_ < 0)
		m_timer_id_ = startTimer(40);

	QPainter p(this);
	p.drawPixmap(0, 0, m_sprites_[m_act_frame_]);
}

void AnimWorking::timerEvent(QTimerEvent *e)
{
	if(m_act_frame_ < TOP_FRAME - 1) m_act_frame_++;
	else m_act_frame_ = 0;
	update();

	if(!isVisible())
	{
		killTimer(m_timer_id_);
		m_timer_id_ = -1;
	}

}
