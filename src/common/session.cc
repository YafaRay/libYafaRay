/****************************************************************************
 *      session.cc: YafaRay Session control
 *      This is part of the libYafaRay package
 *      Copyright (C) 2016 David Bluecame
 *      Session control and persistent objects between renders
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
#include "common/session.h"
#include "photon/photon.h"

#if defined(_WIN32)
#include <windows.h>
#endif

BEGIN_YAFARAY

// Initialization of the master instance of yafLog
Logger logger__ = Logger();

// Initialization of the master session instance
Session session__ = Session();

Session::Session(const Session &)	//We need to redefine the copy constructor to avoid trying to copy the mutex (not copiable). This copy constructor will not copy anything, but we only have one session object anyway so it should be ok.
{
}

Session::Session()
{
	Y_VERBOSE << "Session:started" << YENDL;
#if defined(_WIN32)
	SetConsoleOutputCP(65001);	//set Windows Console to UTF8 so the image path can be displayed correctly
#endif
	caustic_map_ = new PhotonMap;
	caustic_map_->setName("Caustic Photon Map");
	diffuse_map_ = new PhotonMap;
	diffuse_map_->setName("Diffuse Photon Map");
	radiance_map_ = new PhotonMap;
	radiance_map_->setName("FG Radiance Photon Map");
}

Session::~Session()
{
	delete radiance_map_;
	delete diffuse_map_;
	delete caustic_map_;
	Y_VERBOSE << "Session: ended" << YENDL;
}

void Session::setStatusRenderStarted()
{
	mutx_.lock();

	render_in_progress_ = true;
	render_finished_ = false;
	render_resumed_ = false;
	render_aborted_ = false;
	total_passes_ = 0;
	current_pass_ = 0;
	current_pass_percent_ = 0.f;

	mutx_.unlock();
}

void Session::setStatusRenderResumed()
{
	mutx_.lock();

	render_in_progress_ = true;
	render_finished_ = false;
	render_resumed_ = true;
	render_aborted_ = false;

	mutx_.unlock();
}

void Session::setStatusRenderFinished()
{
	mutx_.lock();

	render_in_progress_ = false;
	render_finished_ = true;

	mutx_.unlock();
}

void Session::setStatusRenderAborted()
{
	mutx_.lock();

	render_in_progress_ = false;
	render_aborted_ = true;

	mutx_.unlock();
}

void Session::setStatusTotalPasses(int total_passes)
{
	mutx_.lock();
	total_passes_ = total_passes;
	mutx_.unlock();
}

void Session::setStatusCurrentPass(int current_pass)
{
	mutx_.lock();
	current_pass_ = current_pass;
	mutx_.unlock();
}

void Session::setStatusCurrentPassPercent(float current_pass_percent)
{
	mutx_.lock();
	current_pass_percent_ = current_pass_percent;
	mutx_.unlock();
}

void Session::setInteractive(bool interactive)
{
	mutx_.lock();
	interactive_ = interactive;
	mutx_.unlock();
}

void Session::setPathYafaRayXml(std::string path)
{
	mutx_.lock();
	path_yafa_ray_xml_ = path;
	mutx_.unlock();
}

void Session::setPathImageOutput(std::string path)
{
	mutx_.lock();
	path_image_output_ = path;
	mutx_.unlock();
}

bool Session::renderInProgress()
{
	return render_in_progress_;
}

bool Session::renderResumed()
{
	return render_resumed_;
}

bool Session::renderFinished()
{
	return render_finished_;
}

bool Session::renderAborted()
{
	return render_aborted_;
}

int Session::totalPasses()
{
	return total_passes_;
}

int Session::currentPass()
{
	return current_pass_;
}

float Session::currentPassPercent()
{
	return current_pass_percent_;
}

bool Session::isInteractive()
{
	return interactive_;
}

std::string Session::getPathYafaRayXml()
{
	return path_yafa_ray_xml_;
}

std::string Session::getPathImageOutput()
{
	return path_image_output_;
}

END_YAFARAY

