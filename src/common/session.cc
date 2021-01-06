/****************************************************************************
 *      This is part of the libYafaRay package
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
Logger logger_global{ };

// Initialization of the master session instance
Session session_global{ };

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

void Session::setInteractive(bool interactive)
{
	std::lock_guard<std::mutex> lock_guard(mutx_);
	interactive_ = interactive;
}

bool Session::isInteractive()
{
	return interactive_;
}

END_YAFARAY

