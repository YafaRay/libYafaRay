#pragma once
/****************************************************************************
 *      session.h: YafaRay Session control
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

#ifndef YAFARAY_SESSION_H
#define YAFARAY_SESSION_H

#include "constants.h"
#include "thread.h"

BEGIN_YAFARAY

class PhotonMap;

class LIBYAFARAY_EXPORT Session
{
	public:
		Session();
		Session(const Session &) = delete; //deleting copy constructor so we can use a std::mutex as a class member (not copiable)

		~Session();

		void setInteractive(bool interactive);
		void setDifferentialRaysEnabled(bool value) { ray_differentials_enabled_ = value; }

		bool getDifferentialRaysEnabled() const { return ray_differentials_enabled_; }
		bool isPreview() const { return false; } //FIXME!

		bool isInteractive();

		PhotonMap *caustic_map_ = nullptr;
		PhotonMap *diffuse_map_ = nullptr;
		PhotonMap *radiance_map_ = nullptr;

		std::mutex mutx_;

	protected:
		bool ray_differentials_enabled_ = false;  //!< By default, disable ray differential calculations. Only if at least one texture uses them, then enable differentials. This should avoid the (many) extra calculations when they are not necessary.
		bool interactive_ = false;
};

extern LIBYAFARAY_EXPORT Session session_global;

END_YAFARAY

#endif
