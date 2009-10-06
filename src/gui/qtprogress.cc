/****************************************************************************
 *      progressbar.h: A progress updater for the yafray GUI
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

#include "qtprogress.h"
#include "events.h"
#include "mywindow.h"
#include <QtCore/QCoreApplication>

QtProgress::QtProgress(MainWindow *window, int cwidth)
: yafaray::ConsoleProgressBar_t(cwidth), m_win(window), m_currentStep(0), m_totalSteps(0)
{
	
}

QtProgress::~QtProgress()
{
}

void QtProgress::init(int totalSteps)
{
	m_currentStep = 0;
	m_totalSteps = totalSteps;
	QCoreApplication::postEvent(m_win, new ProgressUpdateEvent(0, 0, totalSteps));

	//yafaray::ConsoleProgressBar_t::init(totalSteps);
}

void QtProgress::update(int steps)
{
	m_currentStep += steps;
	QCoreApplication::postEvent(m_win, new ProgressUpdateEvent(m_currentStep));

	//yafaray::ConsoleProgressBar_t::update(steps);
}

void QtProgress::done()
{
	m_currentStep = m_totalSteps;
	QCoreApplication::postEvent(m_win, new ProgressUpdateEvent(m_currentStep));
	
	//yafaray::ConsoleProgressBar_t::done();
}

void QtProgress::setTag(const char *tag)
{
	QCoreApplication::postEvent(m_win, new ProgressUpdateTagEvent(tag));
	
	//yafaray::ConsoleProgressBar_t::done();
}
