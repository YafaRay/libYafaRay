/****************************************************************************
 *      mywindow.h: main window of the yafray UI
 *      This is part of the libYafaRay package
 *      Copyright (C) 2008 Gustavo Pichorim Boiko
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


#ifndef YAFARAY_MYWINDOW_H
#define YAFARAY_MYWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <string>

#include "gui/interface_qt.h"

class Ui_WindowBase;

namespace yafaray4
{
class Interface;
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
		MainWindow(yafaray4::Interface *interface, int resx, int resy, int b_start_x, int b_start_y, Settings settings);
		~MainWindow();

		virtual bool event(QEvent *e);
		void adjustWindow();

	protected:
		virtual bool eventFilter(QObject *obj, QEvent *event);
		virtual void keyPressEvent(QKeyEvent *event);
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
		Ui_WindowBase *ui_;
		RenderWidget *render_;
		QtOutput *output_;
		Worker *worker_;
		yafaray4::Interface *interface_;
		QString output_path_;
		QString last_path_;
		int res_x_, res_y_, b_x_, b_y_;
		std::string file_name_;
		bool auto_close_;	// if true, rendering gets saved to fileName after finish and GUI gets closed (for animation)
		bool auto_save_;	// if true, rendering gets saved to fileName after finish but GUI stays opened
		bool auto_save_alpha_;	// if true, the automatically saved image contains no alpha channel
		bool save_with_alpha_;
		bool save_with_depth_;
		bool use_draw_params_;
		QTime time_measure_;		// time measure for the render
		AnimWorking *anim_;
		bool render_saved_;
		bool render_cancelled_;
		bool use_zbuf_;
		bool ask_unsaved_;
};

#endif
