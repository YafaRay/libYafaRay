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
#include <yafraycore/photon.h>

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

__END_YAFRAY

