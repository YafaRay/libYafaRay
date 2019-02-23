/****************************************************************************
 *      worker.h: a thread for doing the rendering process
 *      This is part of the yafray package
 *      Copyright (C) 2008 Gustavo Pichorim Boiko
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

#ifndef WORKER_H
#define WORKER_H

#include <QThread>
#include <QString>

class QtOutput;
class MainWindow;
namespace yafaray
{
class yafrayInterface_t;
class render_t;
class paraMap_t;
class scene_t;
}


class Worker : public QThread
{
	Q_OBJECT
public:
	Worker(yafaray::yafrayInterface_t *env, MainWindow *w, QtOutput *output);
	void run();
private:
	yafaray::yafrayInterface_t *m_env;
	QtOutput *m_output;
	MainWindow *m_win;
	bool m_valid;
};

#endif
