/****************************************************************************
 *      mywindow.h: main window of the yafray UI
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


#ifndef MYWINDOW_H
#define MYWINDOW_H

#include <QtGui/QMainWindow>
#include <QtCore/QTime>
#include <QtGui/QProgressBar>
#include <string>

#include <yafraycore/memoryIO.h>
#include "yafqtapi.h"

namespace Ui
{
class WindowBase;
}

namespace yafaray
{
	class yafrayInterface_t;
}

class AnimWorking;
class QtOutput;
class RenderWidget;
class Worker;
class QErrorMessage;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	//MainWindow(const char *filename = 0);
	MainWindow(yafaray::yafrayInterface_t *interf, int resx, int resy, int bStartX, int bStartY, Settings settings);
	~MainWindow();

	virtual bool event(QEvent *e);
	void adjustWindow();

protected:
	virtual bool eventFilter(QObject *obj, QEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);

public slots:
	void slotRender();
	void slotFinished();
	void slotEnableDisable(bool enable = true);
	void slotOpen();
	void slotSave();
	void slotSaveAs();
	void slotUseAlpha(int state);
	void slotCancel();
	void close();
	void zoomIn();
	void zoomOut();

private:
	Ui::WindowBase *m_ui;
	RenderWidget *m_render;
	QtOutput *m_output;
	Worker *m_worker;
	QErrorMessage *errorMessage;
	yafaray::yafrayInterface_t *interf;
	QString m_outputPath;
	QString m_lastPath;
	int res_x, res_y;
	std::string fileName;
	bool autoClose;	// if true, rendering gets saved to fileName after finish and GUI gets closed (for animation)
	bool autoSave;	// if true, rendering gets saved to fileName after finish but GUI stays opened
	bool autoSaveAlpha;	// if true, the automatically saved image contains no alpha channel
	yafaray::memoryIO_t* memIO;	// if not NULL, the image also is saved into this memory buffer
	QTime timeMeasure;		// time measure for the render
	QProgressBar* progressbar;
	AnimWorking* anim;
};

#endif
