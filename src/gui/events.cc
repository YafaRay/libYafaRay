/****************************************************************************
 *      events.cc: custom events to enable thread communication to the UI
 *      This is part of the libYafaRay package
 *      Copyright (C) 2009 Gustavo Pichorim Boiko
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

#include "events.h"

GuiUpdateEvent::GuiUpdateEvent(const QRect &rect, bool full_update)
	: QEvent((QEvent::Type)GuiUpdate), m_rect_(rect), m_full_(full_update)
{
}

GuiAreaHighliteEvent::GuiAreaHighliteEvent(const QRect &rect)
	: QEvent((QEvent::Type)GuiAreaHighlite), m_rect_(rect)
{
}

ProgressUpdateEvent::ProgressUpdateEvent(int progress, int min, int max)
	: QEvent((QEvent::Type)ProgressUpdate), m_progress_(progress), m_min_(min), m_max_(max)
{
}

ProgressUpdateTagEvent::ProgressUpdateTagEvent(const char *tag)
	: QEvent((QEvent::Type)ProgressUpdateTag), m_tag_(tag)
{
}
