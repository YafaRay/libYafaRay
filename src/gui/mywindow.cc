/****************************************************************************
 *      mywindow.cc: the main window for the yafray GUI
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

#include "yafqtapi.h"
#include "mywindow.h"
#include "worker.h"
#include "qtoutput.h"
#include "renderwidget.h"
#include "windowbase.h"
#include "events.h"
#include "animworking.h"
#include <interface/yafrayinterface.h>
#include <yafraycore/EXR_io.h>
#include <yafraycore/memoryIO.h>
#include "yafarayicon.h"
#include "guifont.h"

#include <iostream>

#include <QtGui/QHBoxLayout>
#include <QtGui/QScrollArea>
#include <QtGui/QFileDialog>
#include <QtCore/QDir>
#include <QtGui/QImageWriter>
#include <QtGui/QErrorMessage>
#include <QtGui/QKeyEvent>
#include <QtGui/QDesktopWidget>
#include <QtGui/QFontDatabase>
#include <string>


static QApplication *app=0;

/**************************
 *
 * yafqtapi functions
 *
 *************************/

void initGui()
{
	static int argc=0;

	if(!QApplication::instance())
	{
		std::cout << "creating new QApplication\n";
		app = new QApplication(argc, 0);
	}
	else app = static_cast<QApplication*>(QApplication::instance());
}

int createRenderWidget(yafaray::yafrayInterface_t *interf, int xsize, int ysize, int bStartX, int bStartY, Settings settings)
{
	MainWindow w(interf, xsize, ysize, bStartX, bStartY, settings);
	w.show();
	w.adjustWindow();
	w.slotRender();

	return app->exec();
}


MainWindow::MainWindow(yafaray::yafrayInterface_t *env, int resx, int resy, int bStartX, int bStartY, Settings settings)
: QMainWindow(), interf(env), res_x(resx), res_y(resy)
{
	QPixmap yafIcon;

	yafIcon.loadFromData(yafarayicon, yafarayicon_size);
	
	int fId = QFontDatabase::addApplicationFontFromData(QByteArray(guifont, guifont_size));
	QStringList fam = QFontDatabase::applicationFontFamilies(fId);
	QFont gFont = QFont(fam[0]);
	gFont.setPointSize(8);
	QApplication::setFont(gFont);
	
	m_ui = new Ui::WindowBase();
	m_ui->setupUi(this);
	
	setWindowIcon(QIcon(yafIcon));

	m_render = new RenderWidget(m_ui->renderArea);
	m_output = new QtOutput(m_render);
	m_worker = new Worker(env, this, m_output);
	errorMessage = new QErrorMessage(this);

	m_output->setRenderSize(QSize(resx, resy));

	// create the progressbar and hide it
	progressbar = new QProgressBar(m_ui->centralwidget);
	progressbar->setObjectName("yafPB");
	progressbar->setMinimum(0);
	progressbar->setMaximum(10);
	progressbar->setTextVisible(false);
	progressbar->setMaximumHeight(13);
	progressbar->setMinimumHeight(13);
	m_ui->gridLayout->addWidget(progressbar,2,0,1,5);
	
	// animation widget
	anim = new AnimWorking(this);
	anim->resize(70,70);

	this->move(20, 20);

	m_ui->renderArea->setWidgetResizable(false);
	m_ui->renderArea->resize(resx, resy);
	m_ui->renderArea->setWidget(m_render);

	QPalette renderAreaPal;
	renderAreaPal = m_ui->renderArea->viewport()->palette();
	renderAreaPal.setColor(QPalette::Background, Qt::black);
	
	m_ui->renderArea->viewport()->setPalette(renderAreaPal);
	
	connect(m_ui->renderButton, SIGNAL(clicked()), this, SLOT(slotRender()));
	connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(slotCancel()));
	connect(m_ui->quitButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(m_worker, SIGNAL(finished()), this, SLOT(slotFinished()));
	connect(app, SIGNAL(aboutToQuit()), this, SLOT(slotCancel()));

	// move the animwidget over the render area
	QRect r = anim->rect();
	r.moveCenter(m_ui->renderArea->rect().center());
	anim->move(r.topLeft());

	connect(m_ui->alphaCheck, SIGNAL(stateChanged(int)), this, SLOT(slotUseAlpha(int)));

	// actions
	connect(m_ui->actionOpen, SIGNAL(triggered(bool)),
			this, SLOT(slotOpen()));
	connect(m_ui->actionSave, SIGNAL(triggered(bool)),
			this, SLOT(slotSave()));
	connect(m_ui->actionSave_As, SIGNAL(triggered(bool)),
			this, SLOT(slotSaveAs()));
	connect(m_ui->actionQuit, SIGNAL(triggered(bool)),
			this, SLOT(close()));
	connect(m_ui->actionZoom_In, SIGNAL(triggered(bool)),
			this, SLOT(zoomIn()));
	connect(m_ui->actionZoom_Out, SIGNAL(triggered(bool)),
			this, SLOT(zoomOut()));

	// offset when using border rendering
	m_render->borderStart = QPoint(bStartX, bStartY);

	memIO = NULL;
	if (settings.mem)
		memIO = new yafaray::memoryIO_t(resx, resy, settings.mem);
	autoSave = settings.autoSave;
	autoSaveAlpha = settings.autoSaveAlpha;
	autoClose = settings.closeAfterFinish;

	if (autoSave) {
		this->fileName = settings.fileName;
		this->setWindowTitle(this->windowTitle() + QString(" (") + QString(fileName.c_str()) + QString(")"));
	}

	// filter the resize events of the render area to center the animation widget
	m_ui->renderArea->installEventFilter(this);
}

MainWindow::~MainWindow()
{
	delete m_output;
	delete m_render;
	delete m_worker;
	delete m_ui;
	delete errorMessage;
}

bool MainWindow::event(QEvent *e)
{
	if (e->type() == (QEvent::Type)ProgressUpdate)
	{
		if (anim->isVisible())
			anim->hide();

		ProgressUpdateEvent *p = static_cast<ProgressUpdateEvent*>(e);
		if (p->min() >= 0)
			progressbar->setMinimum(p->min());
		if (p->max() >= 0)
			progressbar->setMaximum(p->max());
		progressbar->setValue(p->progress());
		return true;
	}

	return QMainWindow::event(e);
}

void MainWindow::slotRender()
{
	slotEnableDisable(false);
	timeMeasure.start();

	m_worker->start();
}

void MainWindow::slotFinished()
{
	if (autoSave) {
		std::cout << "INFO: Image saved to " << fileName;
		if (autoSaveAlpha) std::cout << " with alpha" << std::endl;
		else std::cout << " without alpha" << std::endl;
		m_render->saveImage(QString(fileName.c_str()), autoSaveAlpha);
	}

	if (autoClose) {
		app->exit(0);
	}
	else {
		int renderTime = timeMeasure.elapsed();
		float timeSec = renderTime / 1000.f;
		
		int ms = renderTime % 1000;
		renderTime = renderTime / 1000;
		int s = renderTime % 60;
		renderTime = renderTime / 60;
		int m = renderTime % 60;
		int h = renderTime / 60;
		
		QString timeStr = "";
		QChar fill = '0';
		QString suffix = "";
		
		if(h > 0)
		{
			timeStr.append(QString("%1:").arg(h));
			suffix = "h.";
		}
		
		if(m > 0 || h > 0)
		{
			if(h == 0) timeStr.append(QString("%1:").arg(m, 2, 10, fill));
			else timeStr.append(QString("%1:").arg(m));
			
			if(suffix == "") suffix = "m.";
		}
		
		if(s < 10 && m == 0 && h == 0) timeStr.append(QString("%1.%2").arg(s).arg(ms, 3, 10, fill));
		else timeStr.append(QString("%1.%2").arg(s, 2, 10, fill).arg(ms, 3, 10, fill));

		if(suffix == "") suffix = "s.";
		
		timeStr.append(QString(" %1").arg(suffix));
		
		
		QString rt = QString("Render time: %1 [%2 s.]").arg(timeStr).arg(timeSec, 5);
		m_ui->statusbar->showMessage(rt);
		std::cout << "finished, setting pixmap" << std::endl;
		m_render->finishedRender();
		slotEnableDisable(true);
	}
}

void MainWindow::slotEnableDisable(bool enable)
{
	m_ui->renderButton->setEnabled(enable);
	m_ui->cancelButton->setEnabled(!enable);
 	m_ui->actionZoom_In->setEnabled(enable);
 	m_ui->actionZoom_Out->setEnabled(enable);
}

void MainWindow::slotOpen()
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
}

void MainWindow::slotSave()
{
	if (m_outputPath.isNull())
	{
		slotSaveAs();
		return;
	}
}

void MainWindow::slotSaveAs()
{
	QString formats;
	QList<QByteArray> formatList;
	QList<QByteArray> formatDesc;
	formatList << "png" << "tga" << "jpeg" << "tiff" << "bmp";
	formatDesc << "PNG" << "TGA" << "JPEG" << "TIFF" << "BMP"; // could be actual descriptions, but who knows them anyway ;-)
	QList<QByteArray> qtFormats = QImageWriter::supportedImageFormats();
	for (int i = 0; i < formatList.size(); ++i) {
		QByteArray format = formatList.at(i);
		QByteArray desc = formatDesc.at(i);
		if (qtFormats.contains(format)) {
			formats += QString(desc) + " (*." + QString(format) + ")" + ";;";
		}
	}
#if HAVE_EXR
	formats += "EXR (*.exr)";
#endif

	if (m_lastPath.isNull())
		m_lastPath = QDir::currentPath();

	QString selectedFilter;

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), m_lastPath,
			formats, &selectedFilter);

	// "re"extract the actual file ending
	selectedFilter.remove(0, selectedFilter.indexOf("."));
	selectedFilter.remove(selectedFilter.indexOf(")"), 2);
	if (!fileName.endsWith(selectedFilter, Qt::CaseInsensitive))
		fileName += selectedFilter.toLower();

	if (!fileName.isNull())
	{
		m_lastPath = QDir(fileName).absolutePath();
		if(fileName.endsWith(".exr", Qt::CaseInsensitive))
		{
#if HAVE_EXR
			//std::cout << "saving EXR file\n" << qPrintable(fileName) << std::endl;
			std::string fname = m_lastPath.toStdString();
			yafaray::outEXR_t exrout(res_x, res_y, fname.c_str(), "");
			interf->getRenderedImage(exrout);
#else
			errorMessage->showMessage(tr("This build has been compiled without OpenEXR."));
#endif
		}
		else if (m_render->saveImage(fileName, m_ui->alphaCheck->isChecked()))
		{
			m_outputPath = fileName;
		}
		// TODO: show error message on !saving
	}
}

void MainWindow::slotUseAlpha(int state)
{
	/* 	bool alpha = (state == Qt::Checked);

	if (alpha != m_output->useAlpha())
		m_output->setUseAlpha(alpha); */
}

void MainWindow::slotCancel() {
	// cancel the render and cleanup, especially wait for the worker to finish up
	// (otherwise the app will crash (if this is followed by a quit))
	interf->abort();
	m_worker->wait();

	// before closing, transfer the render result into the memory buffer
	// if it exists
	if (memIO)
		interf->getRenderedImage(*memIO);
}

void MainWindow::close() {
	// this will call slotCancel as well, since it's slotted into aboutToQuit signal
	app->quit();
}


void MainWindow::keyPressEvent(QKeyEvent* event) {
	if (event->key() == Qt::Key_Escape) {
		app->exit(1);
	}
}

bool MainWindow::eventFilter(QObject *obj, QEvent* event)
{
	if (event->type() == QEvent::Resize)
	{
		// move the animwidget over the render area
		QRect r = anim->rect();
		r.moveCenter(m_ui->renderArea->rect().center());
		anim->move(r.topLeft());
	}
	return QMainWindow::eventFilter(obj, event);
}

void MainWindow::zoomOut() {
	m_render->zoomOut(QPoint(0,0));
}

void MainWindow::zoomIn() {
	m_render->zoomIn(QPoint(0,0));
}

void MainWindow::adjustWindow() {
	int offset = 40;
 	QRect scrGeom = QApplication::desktop()->availableGeometry();

	int w = std::min(res_x + 10, scrGeom.width()-offset);
	int h = std::min(res_y + 10, scrGeom.height()-offset*3);
	
	std::cout << "w,h = " << w << ", " << h << "\n";
	
	m_ui->renderArea->setMaximumSize(w, h);
	m_ui->renderArea->setMinimumSize(w, h);

	adjustSize();
	resize(minimumSize());
	
	m_ui->renderArea->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	m_ui->renderArea->setMinimumSize(0, 0);
	
}
