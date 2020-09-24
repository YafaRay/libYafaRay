/****************************************************************************
 *      mywindow.cc: the main window for the yafray GUI
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

// YafaRay Headers
#include "common/logging.h"
#include "color/color_console.h"
#include "gui/interface_qt.h"
#include "mywindow.h"
#include "worker.h"
#include "qtoutput.h"
#include "renderwidget.h"
#include "ui_windowbase.h"
#include "events.h"
#include "animworking.h"
#include "interface/interface.h"
#include "common/param.h"
#include "output/output_image.h"
#include "imagehandler/imagehandler.h"

// Embeded Resources:

// Images
#include "resource/yafarayicon.h"
#include "resource/toolbar_z_buffer_icon.h"
#include "resource/toolbar_alpha_icon.h"
#include "resource/toolbar_cancel_icon.h"
#include "resource/toolbar_save_as_icon.h"
#include "resource/toolbar_render_icon.h"
#include "resource/toolbar_show_alpha_icon.h"
#include "resource/toolbar_colorbuffer_icon.h"
#include "resource/toolbar_drawparams_icon.h"
#include "resource/toolbar_savedepth_icon.h"
#include "resource/toolbar_zoomin_icon.h"
#include "resource/toolbar_zoomout_icon.h"
#include "resource/toolbar_quit_icon.h"

// GUI Font
#if !defined(__APPLE__) && defined(YAFQT_EMBEDED_FONT)
#include "resource/guifont.h"
#endif

// End of resources inclusion

// Standard Headers
#include <iostream>
#include <string>
#include <algorithm>

// Qt Headers
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QtCore/QDir>
#include <QImageWriter>
#include <QErrorMessage>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QSettings>

static QApplication *app__ = nullptr;

/**************************
 *
 * yafqtapi functions
 *
 *************************/

void initGui__()
{
	static int argc = 0;

	if(!QApplication::instance())
	{
#if defined(__APPLE__)
		QApplication::instance()->setAttribute(Qt::AA_MacPluginApplication);
		QApplication::instance()->setAttribute(Qt::AA_DontUseNativeMenuBar);
#endif
		using namespace yafaray4;
		Y_INFO << "Starting Qt graphical interface..." << YENDL;
		app__ = new QApplication(argc, 0);
	}
	else app__ = static_cast<QApplication *>(QApplication::instance());
}

int createRenderWidget__(yafaray4::Interface *interf, int xsize, int ysize, int b_start_x, int b_start_y, Settings settings)
{
	MainWindow w(interf, xsize, ysize, b_start_x, b_start_y, settings);
	w.show();
	w.adjustWindow();
	w.slotRender();

	return app__->exec();
}


MainWindow::MainWindow(yafaray4::Interface *interface, int resx, int resy, int b_start_x, int b_start_y, Settings settings)
	: QMainWindow(), interface_(interface), res_x_(resx), res_y_(resy), b_x_(b_start_x), b_y_(b_start_y), use_zbuf_(false)
{
	QCoreApplication::setOrganizationName("YafaRay Team");
	QCoreApplication::setOrganizationDomain("yafaray.org");
	QCoreApplication::setApplicationName("YafaRay Qt Gui");

	QSettings set;

	ask_unsaved_ = set.value("qtGui/askSave", true).toBool();

	QPixmap yaf_icon;
	QPixmap zbuff_icon;
	QPixmap alpha_icon;
	QPixmap cancel_icon;
	QPixmap save_as_icon;
	QPixmap render_icon;
	QPixmap show_alpha_icon;
	QPixmap show_color_icon;
	QPixmap save_depth_icon;
	QPixmap draw_params_icon;
	QPixmap zoom_in_icon;
	QPixmap zoom_out_icon;
	QPixmap quit_icon;

	yaf_icon.loadFromData(yafarayicon__, yafarayicon_size__);
	zbuff_icon.loadFromData(z_buf_icon__, z_buf_icon_size__);
	alpha_icon.loadFromData(alpha_icon__, alpha_icon_size__);
	cancel_icon.loadFromData(cancel_icon__, cancel_icon_size__);
	save_as_icon.loadFromData(saveas_icon__, saveas_icon_size__);
	render_icon.loadFromData(render_icon__, render_icon_size__);
	show_alpha_icon.loadFromData(show_alpha_icon__, show_alpha_icon_size__);
	show_color_icon.loadFromData(rgb_icon__, rgb_icon_size__);
	save_depth_icon.loadFromData(save_z_buf_icon__, save_z_buf_icon_size__);
	draw_params_icon.loadFromData(drawparams_icon__, drawparams_icon_size__);
	zoom_in_icon.loadFromData(zoomin_icon__, zoomin_icon_size__);
	zoom_out_icon.loadFromData(zoomout_icon__, zoomout_icon_size__);
	quit_icon.loadFromData(quit_icon__, quit_icon_size__);

#if !defined(__APPLE__) && defined(YAFQT_EMBEDED_FONT)
	int fId = QFontDatabase::addApplicationFontFromData(QByteArray((const char *)guifont, guifont_size));
	QStringList fam = QFontDatabase::applicationFontFamilies(fId);
	QFont gFont = QFont(fam[0]);
	gFont.setPointSize(8);
	QApplication::setFont(gFont);
#else
#if defined(__APPLE__)
	QFont gFont = QApplication::font();
	gFont.setPointSize(13);
	QApplication::setFont(gFont);
#endif
#endif

	ui_ = new Ui_WindowBase();
	ui_->setupUi(this);

	setWindowIcon(QIcon(yaf_icon));

#if defined(__APPLE__)
	m_ui->menubar->setNativeMenuBar(false); //Otherwise the menus don't appear in MacOS for some weird reason
	m_ui->toolBar->close(); //FIXME: I was unable to make the icons in the tool bar to show in MacOS, really weird... so for now we just hide the toolbar entirely in Mac
#endif

	render_saved_ = false;
	render_cancelled_ = false;
	save_with_alpha_ = false;

	ui_->actionAskSave->setChecked(ask_unsaved_);

	yafaray4::ParamMap *p = interface_->getRenderParameters();
	p->getParam("z_channel", use_zbuf_);

	render_ = new RenderWidget(ui_->renderArea, use_zbuf_);
	output_ = new QtOutput(render_);
	worker_ = new Worker(interface_, this, output_);

	output_->setRenderSize(QSize(resx, resy));

	// animation widget
	anim_ = new AnimWorking(ui_->renderArea);
	anim_->resize(200, 87);

	this->move(20, 20);

	ui_->renderArea->setWidgetResizable(false);
	ui_->renderArea->resize(resx, resy);
	ui_->renderArea->setWidget(render_);

	QPalette render_area_pal;
	render_area_pal = ui_->renderArea->viewport()->palette();
	render_area_pal.setColor(QPalette::Window, Qt::black);

	ui_->renderArea->viewport()->setPalette(render_area_pal);

	ui_->cancelButton->setIcon(QIcon(cancel_icon));

	connect(ui_->cancelButton, SIGNAL(clicked()), this, SLOT(slotCancel()));
	connect(worker_, SIGNAL(finished()), this, SLOT(slotFinished()));

	// move the animwidget over the render area
	QRect r = anim_->rect();
	r.moveCenter(ui_->renderArea->rect().center());
	anim_->move(r.topLeft());

	// Set toolbar icons
	ui_->actionSaveAlpha->setIcon(QIcon(alpha_icon));
	ui_->actionCancel->setIcon(QIcon(cancel_icon));
	ui_->actionSave_As->setIcon(QIcon(save_as_icon));
	ui_->actionRender->setIcon(QIcon(render_icon));
	ui_->actionShowAlpha->setIcon(QIcon(show_alpha_icon));
	ui_->actionShowRGB->setIcon(QIcon(show_color_icon));
	ui_->actionDrawParams->setIcon(QIcon(draw_params_icon));
	ui_->actionZoom_In->setIcon(QIcon(zoom_in_icon));
	ui_->actionZoom_Out->setIcon(QIcon(zoom_out_icon));
	ui_->actionQuit->setIcon(QIcon(quit_icon));

	// actions
	connect(ui_->actionRender, SIGNAL(triggered(bool)),
			this, SLOT(slotRender()));
	connect(ui_->actionCancel, SIGNAL(triggered(bool)),
			this, SLOT(slotCancel()));
	connect(ui_->actionSave_As, SIGNAL(triggered(bool)),
			this, SLOT(slotSaveAs()));
	connect(ui_->actionQuit, SIGNAL(triggered(bool)),
			this, SLOT(close()));
	connect(ui_->actionZoom_In, SIGNAL(triggered(bool)),
			this, SLOT(zoomIn()));
	connect(ui_->actionZoom_Out, SIGNAL(triggered(bool)),
			this, SLOT(zoomOut()));
	connect(ui_->actionSaveAlpha, SIGNAL(triggered(bool)),
			this, SLOT(setAlpha(bool)));
	connect(ui_->actionShowAlpha, SIGNAL(triggered(bool)),
			this, SLOT(showAlpha(bool)));
	connect(ui_->actionShowRGB, SIGNAL(triggered(bool)),
			this, SLOT(showColor(bool)));
	connect(ui_->actionAskSave, SIGNAL(triggered(bool)),
			this, SLOT(setAskSave(bool)));
	//FIXME: connect(m_ui->actionDrawParams, SIGNAL(triggered(bool)),
	//FIXME:		this, SLOT(setDrawParams(bool)));

	ui_->actionShowRGB->setChecked(true);
	use_draw_params_ = interface_->getDrawParams();
	ui_->actionDrawParams->setChecked(use_draw_params_);

	// offset when using border rendering
	render_->setRenderBorderStart(QPoint(b_start_x, b_start_y));

	auto_save_ = settings.auto_save_;
	auto_save_alpha_ = settings.auto_save_alpha_;
	auto_close_ = settings.close_after_finish_;
	save_with_alpha_ = auto_save_alpha_;

	if(auto_save_)
	{
		this->file_name_ = settings.file_name_;
		this->setWindowTitle(this->windowTitle() + QString(" (") + QString(file_name_.c_str()) + QString(")"));
	}

	// filter the resize events of the render area to center the animation widget
	ui_->renderArea->installEventFilter(this);
}

MainWindow::~MainWindow()
{
	delete output_;
	delete render_;
	delete worker_;
	delete ui_;
}

bool MainWindow::event(QEvent *e)
{
	if(e->type() == (QEvent::Type)ProgressUpdate)
	{
		ProgressUpdateEvent *p = static_cast<ProgressUpdateEvent *>(e);
		if(p->min() >= 0)
			ui_->progressbar->setMinimum(p->min());
		if(p->max() >= 0)
			ui_->progressbar->setMaximum(p->max());
		ui_->progressbar->setValue(p->progress());
		return true;
	}

	if(e->type() == (QEvent::Type)ProgressUpdateTag)
	{
		ProgressUpdateTagEvent *p = static_cast<ProgressUpdateTagEvent *>(e);
		if(p->tag().contains("Rendering")) anim_->hide();
		ui_->yafLabel->setText(p->tag());
		return true;
	}

	return QMainWindow::event(e);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if(!closeUnsaved())
	{
		e->ignore();
	}
	else
	{
		slotCancel();

		if(render_cancelled_) app__->exit(1); //check if a render was stopped and exit with the appropiate code
		e->accept();
	}

}

void MainWindow::slotRender()
{
	slotEnableDisable(false);
	ui_->progressbar->show();
	time_measure_.start();
	ui_->yafLabel->setText(tr("Rendering image..."));
	render_->startRendering();
	ui_->actionShowRGB->setChecked(true);
	ui_->actionShowAlpha->setChecked(false);
	render_saved_ = false;
	render_cancelled_ = false;
	worker_->start();
}

void MainWindow::slotFinished()
{
	using namespace yafaray4;
	QString rt = "";

	if(auto_save_)
	{
		Y_INFO << " Image saved to " << file_name_;
		if(auto_save_alpha_) std::cout << " with alpha" << YENDL;
		else std::cout << " without alpha" << YENDL;

		using namespace yafaray4;

		interface_->paramsClearAll();
		interface_->paramsSetString("type", "png");
		interface_->paramsSetInt("width", res_x_);
		interface_->paramsSetInt("height", res_y_);
		interface_->paramsSetBool("alpha_channel", auto_save_alpha_);
		interface_->paramsSetBool("z_channel", use_zbuf_);

		ImageHandler *ih = interface_->createImageHandler("saver", false);
		ImageOutput *out = new ImageOutput(ih, file_name_, b_x_, b_y_);

		interface_->paramsClearAll();

		interface_->getRenderedImage(0, *out); //FIXME DAVID VIEWS!!

		render_saved_ = true;

		rt = QString("Image Auto-saved. ");

		delete ih;
		delete out;

		if(auto_close_)
		{
			if(render_cancelled_) app__->exit(1);
			else app__->quit();
			return;
		}
	}

	int render_time = time_measure_.elapsed();
	float time_sec = render_time / 1000.f;

	int ms = render_time % 1000;
	render_time = render_time / 1000;
	int s = render_time % 60;
	render_time = render_time / 60;
	int m = render_time % 60;
	int h = render_time / 60;

	QString time_str = "";
	QChar fill = '0';
	QString suffix = "";

	if(h > 0)
	{
		time_str.append(QString("%1:").arg(h));
		suffix = "h.";
	}

	if(m > 0 || h > 0)
	{
		if(h > 0) time_str.append(QString("%1:").arg(m, 2, 10, fill));
		else time_str.append(QString("%1:").arg(m));

		if(suffix == "") suffix = "m.";
	}

	if(s < 10 && m == 0 && h == 0) time_str.append(QString("%1.%2").arg(s).arg(ms, 2, 10, fill));
	else time_str.append(QString("%1.%2").arg(s, 2, 10, fill).arg(ms, 2, 10, fill));

	if(suffix == "") suffix = "s.";

	time_str.append(QString(" %1").arg(suffix));

	rt.append(QString("Render time: %1 [%2s.]").arg(time_str).arg(time_sec, 5));
	ui_->yafLabel->setText(rt);
	Y_INFO << SetColor(Green, true) << "Render completed!" << SetColor() << YENDL;

	render_->finishRendering();
	update();

	slotEnableDisable(true);

	if(auto_close_)
	{
		if(render_cancelled_) app__->exit(1);
		else app__->quit();
		return;
	}

	ui_->progressbar->hide();

	QApplication::alert(this);
}


void MainWindow::slotEnableDisable(bool enable)
{
	ui_->actionRender->setVisible(enable);
	ui_->cancelButton->setVisible(!enable);
	ui_->actionCancel->setVisible(!enable);
	ui_->actionZoom_In->setEnabled(enable);
	ui_->actionZoom_Out->setEnabled(enable);
	ui_->actionDrawParams->setEnabled(enable);
}

void MainWindow::setAlpha(bool checked)
{
	save_with_alpha_ = checked;
}

void MainWindow::showColor(bool checked)
{
	if(checked)
	{
		render_->paintColorBuffer();
		ui_->actionShowAlpha->setChecked(false);
	}
	else ui_->actionShowRGB->setChecked(true);
}

void MainWindow::showAlpha(bool checked)
{
	if(checked)
	{
		render_->paintAlpha();
		ui_->actionShowRGB->setChecked(false);
	}
	else ui_->actionShowAlpha->setChecked(true);
}

void MainWindow::setAskSave(bool checked)
{
	QSettings set;
	ask_unsaved_ = checked;
	set.setValue("qtGui/askSave", ask_unsaved_);
}

void MainWindow::setDrawParams(bool checked)
{
	use_draw_params_ = checked;
	if(!render_->isRendering())
	{
		//FIXME: interf->setDrawParams(useDrawParams);
		interface_->getRenderedImage(0, *output_); //FIXME DAVID VIEWS!!
		showColor(true);
	}
}

/*void MainWindow::slotOpen()
{
	if (m_lastPath.isNull())
		m_lastPath = QDir::currentPath();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Yafaray Scene"), m_lastPath, tr("Yafaray Scenes (*.xml)"));

	if (m_worker->isRunning())
		m_worker->terminate();
	delete m_worker;
	m_worker = new Worker(interf, this, m_output);

	m_lastPath = QDir(fileName).absolutePath();
	slotEnableDisable(true);
}*/

/*void MainWindow::slotSave()
{
	if (m_outputPath.isNull())
	{
		saveDlg();
	}
}*/

void MainWindow::slotSaveAs()
{
	saveDlg();
}

bool MainWindow::saveDlg()
{
	QString formats;
/* FIXME	std::vector<std::string> format_list = interface_->listImageHandlers();
	std::vector<std::string> format_desc = interface_->listImageHandlersFullName();

	std::sort(format_list.begin(), format_list.end());
	std::sort(format_desc.begin(), format_desc.end());

	for(size_t i = 0; i < format_list.size(); ++i)
	{
		formats += QString::fromStdString(format_desc[i]) + " (*." + QString::fromStdString(format_list[i]) + ")";
		if(i < format_list.size() - 1) formats += ";;";
	} */

	if(last_path_.isNull())
		last_path_ = QDir::currentPath();

	QString selected_filter;
	render_saved_ = false;
	QString file_name = QFileDialog::getSaveFileName(this, tr("YafaRay Save Image"), last_path_,
													 formats, &selected_filter);

	// "re"extract the actual file ending
	selected_filter.remove(0, selected_filter.indexOf("."));
	selected_filter.remove(selected_filter.indexOf(")"), 2);

	if(!file_name.endsWith(selected_filter, Qt::CaseInsensitive))
	{
		file_name.append(selected_filter.toLower());
	}

	selected_filter.remove(0, 1); // Remove the dot "."

	if(!file_name.isNull())
	{
		using namespace yafaray4;
		interface_->paramsClearAll();
		interface_->paramsSetString("type", selected_filter.toStdString().c_str());
		interface_->paramsSetInt("width", res_x_);
		interface_->paramsSetInt("height", res_y_);
		interface_->paramsSetBool("alpha_channel", save_with_alpha_);

		last_path_ = QDir(file_name).absolutePath();

		ImageHandler *ih = interface_->createImageHandler("saver", false);
		ImageOutput *out = new ImageOutput(ih, last_path_.toStdString(), b_x_, b_y_);

		interface_->paramsClearAll();

		//FIXME: interf->setDrawParams(useDrawParams);

		interface_->getRenderedImage(0, *out); //FIXME DAVID VIEWS!!

		render_saved_ = true;

		QString savemesg;
		savemesg.append("Render ");
		savemesg.append(((use_zbuf_) ? "(RGBA + Z) " : "(RGBA) "));
		savemesg.append("saved.");

		ui_->yafLabel->setText(savemesg);

		delete ih;
		delete out;
	}

	return render_saved_;
}

bool MainWindow::closeUnsaved()
{
	if(!render_saved_ && !render_->isRendering() && ask_unsaved_)
	{
		QMessageBox msg_box(QMessageBox::Question, "YafaRay Question", "The render hasn't been saved, if you close, it will be lost.",
							QMessageBox::NoButton, this);

		msg_box.setInformativeText("Do you want to save your render before closing?");
		QPushButton *discard = msg_box.addButton("Close without Saving", QMessageBox::DestructiveRole);
		QPushButton *save = msg_box.addButton("Save", QMessageBox::AcceptRole);
		QPushButton *cancel = msg_box.addButton("Cancel", QMessageBox::RejectRole);
		msg_box.setDefaultButton(discard);

		msg_box.exec();

		if(msg_box.clickedButton() == save) return saveDlg();
		else if(msg_box.clickedButton() == cancel) return false;
	}
	return true;
}

void MainWindow::slotCancel()
{
	// cancel the render and cleanup, especially wait for the worker to finish up
	// (otherwise the app will crash (if this is followed by a quit))
	if(render_->isRendering()) render_cancelled_ = true;

	interface_->abort();
	worker_->wait();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Escape)	close();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if(event->type() == QEvent::Resize)
	{
		// move the animwidget over the render area
		QRect r = anim_->rect();
		r.moveCenter(ui_->renderArea->rect().center());
		anim_->move(r.topLeft());
	}
	return QMainWindow::eventFilter(obj, event);
}

void MainWindow::zoomOut()
{
	render_->zoomOut(QPoint(0, 0));
}

void MainWindow::zoomIn()
{
	render_->zoomIn(QPoint(0, 0));
}

void MainWindow::adjustWindow()
{
	QRect scr_geom = QApplication::desktop()->availableGeometry();

	int w = std::min(res_x_ + 10, scr_geom.width() - 60);
	int h = std::min(res_y_ + 10, scr_geom.height() - 160);

	ui_->renderArea->setMaximumSize(w, h);
	ui_->renderArea->setMinimumSize(w, h);

	adjustSize();
	resize(minimumSize());

	ui_->renderArea->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	ui_->renderArea->setMinimumSize(0, 0);
}
