/****************************************************************************
 *      events.h: custom events to enable thread communication to the UI
 *      This is part of the yafray package
 *      Copyright (C) 2009 Gustavo Pichorim Boiko
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

#ifndef EVENTS_H
#define EVENTS_H

#include <QtCore/QEvent>
#include <QtCore/QRect>
#include <QtCore/QString>

enum CustomEvents
{
	GuiUpdate = QEvent::User,
	GuiAreaHighlite,
	ProgressUpdate,
	ProgressUpdateTag
};

// this event is just to trigger an update in a widget from 
// outside the main thread
class GuiUpdateEvent : public QEvent
{
public:
	GuiUpdateEvent(const QRect &rect, bool fullUpdate = false);
	inline QRect rect() { return m_rect; }
	inline bool fullUpdate() { return m_full; }
private:
	QRect m_rect;
	bool m_full;
};

class GuiAreaHighliteEvent : public QEvent
{
public:
	GuiAreaHighliteEvent(const QRect &rect);
	inline QRect rect() { return m_rect; }
private:
	QRect m_rect;
};

class ProgressUpdateEvent : public QEvent
{
public:
	ProgressUpdateEvent(int progress, int min = -1, int max = -1);
	inline int progress() { return m_progress; }
	inline int min() { return m_min; }
	inline int max() { return m_max; }
private:
	int m_progress;
	int m_min;
	int m_max;
};

class ProgressUpdateTagEvent : public QEvent
{
public:
	ProgressUpdateTagEvent(const char *tag);
	inline QString &tag() { return m_tag; }
private:
	QString m_tag;
};

#endif
