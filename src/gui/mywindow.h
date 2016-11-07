/****************************************************************************
 *      mywindow.h: main window of the yafray UI
 *      This is part of the yafray package
 *      Copyright (C) 2008 Gustavo Pichorim Boiko
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


#ifndef MYWINDOW_H
#define MYWINDOW_H

#include <QtGui/QMainWindow>
#include <QtCore/QTime>
#include <string>

#include <gui/yafqtapi.h>

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
	MainWindow(yafaray::yafrayInterface_t *env, int resx, int resy, int bStartX, int bStartY, Settings settings);
	~MainWindow();

	virtual bool event(QEvent *e);
	void adjustWindow();

protected:
	virtual bool eventFilter(QObject *obj, QEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	void closeEvent(QCloseEvent *e);
	bool closeUnsaved();
	bool saveDlg();

public slots:
	void slotRender();
	void slotFinished();
	void slotEnableDisable(bool enable = true);
	void slotSaveAs();
	void slotCancel();
	void setAlpha(bool checked);
	void showColor(bool checked);
	void showAlpha(bool checked);
	//FIXME: void showDepth(bool checked);
	void setAskSave(bool checked);
	//FIXME: void setSaveDepth(bool checked);
	void setDrawParams(bool checked);
	void zoomIn();
	void zoomOut();

private:
	Ui::WindowBase *m_ui;
	RenderWidget *m_render;
	QtOutput *m_output;
	Worker *m_worker;
	yafaray::yafrayInterface_t *interf;
	QString m_outputPath;
	QString m_lastPath;
	int res_x, res_y, b_x, b_y;
	std::string fileName;
	bool autoClose;	// if true, rendering gets saved to fileName after finish and GUI gets closed (for animation)
	bool autoSave;	// if true, rendering gets saved to fileName after finish but GUI stays opened
	bool autoSaveAlpha;	// if true, the automatically saved image contains no alpha channel
	bool saveWithAlpha;
	bool saveWithDepth;
	bool useDrawParams;
	QTime timeMeasure;		// time measure for the render
	AnimWorking* anim;
	bool renderSaved;
	bool renderCancelled;
	bool use_zbuf;
	bool askUnsaved;
};

#endif
