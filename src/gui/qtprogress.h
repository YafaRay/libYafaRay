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

#ifndef YAFARAY_QTPROGRESS_H
#define YAFARAY_QTPROGRESS_H

#include <yafraycore/monitor.h>

class MainWindow;

class QtProgress : public yafaray4::ConsoleProgressBar
{
	public:
		QtProgress(MainWindow *window, int cwidth = 80);
		virtual ~QtProgress();
		virtual void init(int total_steps);
		virtual void update(int steps = 1);
		virtual void setTag(const char *tag);
		virtual void done();
	private:
		MainWindow *win_;
		int current_step_;
		int total_steps_;
};

#endif
