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

session_t::session_t()
{
	Y_VERBOSE << "Session:started" << yendl;
	causticMap = new photonMap_t;
	diffuseMap = new photonMap_t;
	radianceMap = new photonMap_t;
}

session_t::~session_t()
{
	delete radianceMap;
	delete diffuseMap;
	delete causticMap;
	Y_VERBOSE << "Session: ended" << yendl;
}

__END_YAFRAY

