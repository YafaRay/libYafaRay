/****************************************************************************
 *      progressbar.h: A progress updater for the yafray GUI
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

#include "qtprogress.h"
#include "events.h"
#include "mywindow.h"
#include <QCoreApplication>

QtProgress::QtProgress(MainWindow *window, int cwidth)
	: yafaray4::ConsoleProgressBar(cwidth), win_(window), current_step_(0), total_steps_(0)
{

}

QtProgress::~QtProgress()
{
}

void QtProgress::init(int total_steps)
{
	current_step_ = 0;
	total_steps_ = total_steps;
	QCoreApplication::postEvent(win_, new ProgressUpdateEvent(0, 0, total_steps));

	//yafaray4::ConsoleProgressBar_t::init(totalSteps);
}

void QtProgress::update(int steps)
{
	current_step_ += steps;
	QCoreApplication::postEvent(win_, new ProgressUpdateEvent(current_step_));

	//yafaray4::ConsoleProgressBar_t::update(steps);
}

void QtProgress::done()
{
	current_step_ = total_steps_;
	QCoreApplication::postEvent(win_, new ProgressUpdateEvent(current_step_));

	//yafaray4::ConsoleProgressBar_t::done();
}

void QtProgress::setTag(const char *tag)
{
	QCoreApplication::postEvent(win_, new ProgressUpdateTagEvent(tag));

	//yafaray4::ConsoleProgressBar_t::done();
}
