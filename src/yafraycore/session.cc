/****************************************************************************
 *      session.cc: YafaRay Session control
 *      This is part of the yafray package
 *		Copyright (C) 2016 David Bluecame
 * 		Session control and persistent objects between renders
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
#include <yafray_config.h>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <yafraycore/photon.h>
#include <yaf_version.h>
#if defined(_WIN32)
	#include <windows.h>
#endif

__BEGIN_YAFRAY

// Initialization of the master instance of yafLog
yafarayLog_t yafLog = yafarayLog_t();

// Initialization of the master session instance
session_t session = session_t();

session_t::session_t(const session_t&)	//We need to redefine the copy constructor to avoid trying to copy the mutex (not copiable). This copy constructor will not copy anything, but we only have one session object anyway so it should be ok.
{
}

session_t::session_t()
{
	Y_VERBOSE << "Session:started" << yendl;
#if defined(_WIN32)
	SetConsoleOutputCP(65001);	//set Windows Console to UTF8 so the image path can be displayed correctly
#endif
	causticMap = new photonMap_t;
	causticMap->setName("Caustic Photon Map");
	diffuseMap = new photonMap_t;
	diffuseMap->setName("Diffuse Photon Map");
	radianceMap = new photonMap_t;
	radianceMap->setName("FG Radiance Photon Map");
}

session_t::~session_t()
{
	delete radianceMap;
	delete diffuseMap;
	delete causticMap;
	Y_VERBOSE << "Session: ended" << yendl;
}

void session_t::setStatusRenderStarted()
{
	mutx.lock();
	
	mRenderInProgress = true;
	mRenderFinished = false;
	mRenderResumed = false;
	mRenderAborted = false;
	mTotalPasses = 0;
	mCurrentPass = 0;
	mCurrentPassPercent = 0.f;
	
	mutx.unlock();
}

void session_t::setStatusRenderResumed()
{
	mutx.lock();
	
	mRenderInProgress = true;
	mRenderFinished = false;
	mRenderResumed = true;
	mRenderAborted = false;
	
	mutx.unlock();
}

void session_t::setStatusRenderFinished()
{
	mutx.lock();
	
	mRenderInProgress = false;
	mRenderFinished = true;
	
	mutx.unlock();
}

void session_t::setStatusRenderAborted()
{
	mutx.lock();
	
	mRenderInProgress = false;
	mRenderAborted = true;
	
	mutx.unlock();
}

void session_t::setStatusTotalPasses(int total_passes)
{
	mutx.lock();
	mTotalPasses = total_passes;
	mutx.unlock();
}

void session_t::setStatusCurrentPass(int current_pass)
{
	mutx.lock();
	mCurrentPass = current_pass;
	mutx.unlock();
}

void session_t::setStatusCurrentPassPercent(float current_pass_percent)
{
	mutx.lock();
	mCurrentPassPercent = current_pass_percent;
	mutx.unlock();
}

void session_t::setInteractive(bool interactive)
{
	mutx.lock();
	mInteractive = interactive;
	mutx.unlock();
}

void session_t::setPathYafaRayXml(std::string path)
{
	mutx.lock();
	mPathYafaRayXml = path;
	mutx.unlock();
}

void session_t::setPathImageOutput(std::string path)
{
	mutx.lock();
	mPathImageOutput = path;
	mutx.unlock();
}

bool session_t::renderInProgress()
{
	return mRenderInProgress;
}

bool session_t::renderResumed()
{
	return mRenderResumed;
}

bool session_t::renderFinished()
{
	return mRenderFinished;
}

bool session_t::renderAborted()
{
	return mRenderAborted;
}

int session_t::totalPasses()
{
	return mTotalPasses;
}

int session_t::currentPass()
{
	return mCurrentPass;
}

float session_t::currentPassPercent()
{
	return mCurrentPassPercent;
}

bool session_t::isInteractive()
{
	return mInteractive;
}

std::string session_t::getPathYafaRayXml()
{
	return mPathYafaRayXml;
}

std::string session_t::getConfiguredRuntimeSearchPathYafaRayPlugins()
{
	return YAF_RUNTIME_SEARCH_PLUGIN_DIR;
}

std::string session_t::getPathImageOutput()
{
	if(mPathImageOutput.empty())
	{
		std::string tempPathOutput = boost::filesystem::temp_directory_path().string()+"/yafaray";
		Y_WARNING << "Image output path not specified, setting to temporary folder: '" << tempPathOutput << "'" << yendl;
		return tempPathOutput;	//if no image output folder was specified, use the system temporary folder
	}
	else return mPathImageOutput;
	
}

std::string session_t::getYafaRayCoreVersion()
{
	// This preprocessor macro is set by cmake during building in file yaf_version.h.cmake
	// For example: cmake -DYAFARAY_CORE_VERSION="v1.2.3"
	// the intention is to link the YafaRay Core version to the git information obtained, for example with:
	// cmake /yafaray/src/Core -DYAFARAY_CORE_VERSION=`git --git-dir=/yafaray/src/Core/.git --work-tree=/yafaray/src/Core describe --dirty --always --tags --long`

	return YAFARAY_CORE_VERSION;
}

__END_YAFRAY

